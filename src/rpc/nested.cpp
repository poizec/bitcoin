/**
 * Split shell command line into a list of arguments and optionally execute the command(s).
 * Aims to emulate \c bash and friends.
 *
 * - Command nesting is possible with parenthesis; for example: validateaddress(getnewaddress())
 * - Arguments are delimited with whitespace or comma
 * - Extra whitespace at the beginning and end and between arguments will be ignored
 * - Text can be "double" or 'single' quoted
 * - The backslash \c \ is used as escape character
 *   - Outside quotes, any character can be escaped
 *   - Within double quotes, only escape \c " and backslashes before a \c " or another backslash
 *   - Within single quotes, no escaping is possible and no special interpretation takes place
 *
 * @param[in]    node    optional node to execute command on
 * @param[out]   strResult   stringified result from the executed command(chain)
 * @param[in]    strCommand  Command line to split
 * @param[in]    fExecute    set true if you want the command to be executed
 * @param[out]   pstrFilteredOut  Command line, filtered to remove any sensitive data
 */

bool RPCConsole::RPCParseCommandLine(interfaces::Node* node, std::string &strResult, const std::string &strCommand, const bool fExecute, std::string * const pstrFilteredOut, const WalletModel* wallet_model)
{
    std::vector< std::vector<std::string> > stack;
    stack.push_back(std::vector<std::string>());

    enum CmdParseState
    {
        STATE_EATING_SPACES,
        STATE_EATING_SPACES_IN_ARG,
        STATE_EATING_SPACES_IN_BRACKETS,
        STATE_ARGUMENT,
        STATE_SINGLEQUOTED,
        STATE_DOUBLEQUOTED,
        STATE_ESCAPE_OUTER,
        STATE_ESCAPE_DOUBLEQUOTED,
        STATE_COMMAND_EXECUTED,
        STATE_COMMAND_EXECUTED_INNER
    } state = STATE_EATING_SPACES;
    std::string curarg;
    UniValue lastResult;
    unsigned nDepthInsideSensitive = 0;
    size_t filter_begin_pos = 0, chpos;
    std::vector<std::pair<size_t, size_t>> filter_ranges;

    auto add_to_current_stack = [&](const std::string& strArg) {
        if (stack.back().empty() && (!nDepthInsideSensitive) && historyFilter.contains(QString::fromStdString(strArg), Qt::CaseInsensitive)) {
            nDepthInsideSensitive = 1;
            filter_begin_pos = chpos;
        }
        // Make sure stack is not empty before adding something
        if (stack.empty()) {
            stack.push_back(std::vector<std::string>());
        }
        stack.back().push_back(strArg);
    };

    auto close_out_params = [&]() {
        if (nDepthInsideSensitive) {
            if (!--nDepthInsideSensitive) {
                assert(filter_begin_pos);
                filter_ranges.push_back(std::make_pair(filter_begin_pos, chpos));
                filter_begin_pos = 0;
            }
        }
        stack.pop_back();
    };

    std::string strCommandTerminated = strCommand;
    if (strCommandTerminated.back() != '\n')
        strCommandTerminated += "\n";
    for (chpos = 0; chpos < strCommandTerminated.size(); ++chpos)
    {
        char ch = strCommandTerminated[chpos];
        switch(state)
        {
            case STATE_COMMAND_EXECUTED_INNER:
            case STATE_COMMAND_EXECUTED:
            {
                bool breakParsing = true;
                switch(ch)
                {
                    case '[': curarg.clear(); state = STATE_COMMAND_EXECUTED_INNER; break;
                    default:
                        if (state == STATE_COMMAND_EXECUTED_INNER)
                        {
                            if (ch != ']')
                            {
                                // append char to the current argument (which is also used for the query command)
                                curarg += ch;
                                break;
                            }
                            if (curarg.size() && fExecute)
                            {
                                // if we have a value query, query arrays with index and objects with a string key
                                UniValue subelement;
                                if (lastResult.isArray())
                                {
                                    for(char argch: curarg)
                                        if (!IsDigit(argch))
                                            throw std::runtime_error("Invalid result query");
                                    subelement = lastResult[atoi(curarg.c_str())];
                                }
                                else if (lastResult.isObject())
                                    subelement = find_value(lastResult, curarg);
                                else
                                    throw std::runtime_error("Invalid result query"); //no array or object: abort
                                lastResult = subelement;
                            }

                            state = STATE_COMMAND_EXECUTED;
                            break;
                        }
                        // don't break parsing when the char is required for the next argument
                        breakParsing = false;

                        // pop the stack and return the result to the current command arguments
                        close_out_params();

                        // don't stringify the json in case of a string to avoid doublequotes
                        if (lastResult.isStr())
                            curarg = lastResult.get_str();
                        else
                            curarg = lastResult.write(2);

                        // if we have a non empty result, use it as stack argument otherwise as general result
                        if (curarg.size())
                        {
                            if (stack.size())
                                add_to_current_stack(curarg);
                            else
                                strResult = curarg;
                        }
                        curarg.clear();
                        // assume eating space state
                        state = STATE_EATING_SPACES;
                }
                if (breakParsing)
                    break;
            }
            case STATE_ARGUMENT: // In or after argument
            case STATE_EATING_SPACES_IN_ARG:
            case STATE_EATING_SPACES_IN_BRACKETS:
            case STATE_EATING_SPACES: // Handle runs of whitespace
                switch(ch)
            {
                case '"': state = STATE_DOUBLEQUOTED; break;
                case '\'': state = STATE_SINGLEQUOTED; break;
                case '\\': state = STATE_ESCAPE_OUTER; break;
                case '(': case ')': case '\n':
                    if (state == STATE_EATING_SPACES_IN_ARG)
                        throw std::runtime_error("Invalid Syntax");
                    if (state == STATE_ARGUMENT)
                    {
                        if (ch == '(' && stack.size() && stack.back().size() > 0)
                        {
                            if (nDepthInsideSensitive) {
                                ++nDepthInsideSensitive;
                            }
                            stack.push_back(std::vector<std::string>());
                        }

                        // don't allow commands after executed commands on baselevel
                        if (!stack.size())
                            throw std::runtime_error("Invalid Syntax");

                        add_to_current_stack(curarg);
                        curarg.clear();
                        state = STATE_EATING_SPACES_IN_BRACKETS;
                    }
                    if ((ch == ')' || ch == '\n') && stack.size() > 0)
                    {
                        if (fExecute) {
                            // Convert argument list to JSON objects in method-dependent way,
                            // and pass it along with the method name to the dispatcher.
                            UniValue params = RPCConvertValues(stack.back()[0], std::vector<std::string>(stack.back().begin() + 1, stack.back().end()));
                            std::string method = stack.back()[0];
                            std::string uri;
#ifdef ENABLE_WALLET
                            if (wallet_model) {
                                QByteArray encodedName = QUrl::toPercentEncoding(wallet_model->getWalletName());
                                uri = "/wallet/"+std::string(encodedName.constData(), encodedName.length());
                            }
#endif
                            assert(node);
                            lastResult = node->executeRpc(method, params, uri);
                        }

                        state = STATE_COMMAND_EXECUTED;
                        curarg.clear();
                    }
                    break;
                case ' ': case ',': case '\t':
                    if(state == STATE_EATING_SPACES_IN_ARG && curarg.empty() && ch == ',')
                        throw std::runtime_error("Invalid Syntax");

                    else if(state == STATE_ARGUMENT) // Space ends argument
                    {
                        add_to_current_stack(curarg);
                        curarg.clear();
                    }
                    if ((state == STATE_EATING_SPACES_IN_BRACKETS || state == STATE_ARGUMENT) && ch == ',')
                    {
                        state = STATE_EATING_SPACES_IN_ARG;
                        break;
                    }
                    state = STATE_EATING_SPACES;
                    break;
                default: curarg += ch; state = STATE_ARGUMENT;
            }
                break;
            case STATE_SINGLEQUOTED: // Single-quoted string
                switch(ch)
            {
                case '\'': state = STATE_ARGUMENT; break;
                default: curarg += ch;
            }
                break;
            case STATE_DOUBLEQUOTED: // Double-quoted string
                switch(ch)
            {
                case '"': state = STATE_ARGUMENT; break;
                case '\\': state = STATE_ESCAPE_DOUBLEQUOTED; break;
                default: curarg += ch;
            }
                break;
            case STATE_ESCAPE_OUTER: // '\' outside quotes
                curarg += ch; state = STATE_ARGUMENT;
                break;
            case STATE_ESCAPE_DOUBLEQUOTED: // '\' in double-quoted text
                if(ch != '"' && ch != '\\') curarg += '\\'; // keep '\' for everything but the quote and '\' itself
                curarg += ch; state = STATE_DOUBLEQUOTED;
                break;
        }
    }
    if (pstrFilteredOut) {
        if (STATE_COMMAND_EXECUTED == state) {
            assert(!stack.empty());
            close_out_params();
        }
        *pstrFilteredOut = strCommand;
        for (auto i = filter_ranges.rbegin(); i != filter_ranges.rend(); ++i) {
            pstrFilteredOut->replace(i->first, i->second - i->first, "(…)");
        }
    }
    switch(state) // final state
    {
        case STATE_COMMAND_EXECUTED:
            if (lastResult.isStr())
                strResult = lastResult.get_str();
            else
                strResult = lastResult.write(2);
        case STATE_ARGUMENT:
        case STATE_EATING_SPACES:
            return true;
        default: // ERROR to end in one of the other states
            return false;
    }
}
