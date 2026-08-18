#pragma once

#ifdef _WIN32
/**
 * @brief Manages the entire firewall rule setup process: checking, elevation, and creation/update.
 * * If the rule is missing, this function relaunches the application with admin privileges,
 * then relaunches the final instance with original arguments. The process then exits.
 * * @param argc The argument count passed to wmain.
 * @param argv The argument vector passed to wmain.
 * @return int The exit code for the current process instance.
 * Returns -1 if the application should proceed normally.
 * Returns 0 or 1 if the process should exit immediately.
 */
int EnsureFirewallRule(int argc, wchar_t* argv[]);

/**
 * @brief Manages the entire firewall rule setup process: checking, elevation, and creation/update.
 * * If the rule is missing, this function relaunches the application with admin privileges,
 * then relaunches the final instance with original arguments. The process then exits.
 * * This overload handles Narrow (char*) arguments by converting them to Wide (wchar_t*).
 * * @param argc The argument count passed to main.
 * @param argv The argument vector passed to main.
 * @return int The exit code for the current process instance.
 * Returns -1 if the application should proceed normally.
 * Returns 0 or 1 if the process should exit immediately.
 */
int EnsureFirewallRule(int argc, char* argv[]);

/**
 * @brief Manages the entire firewall rule setup process: checking, elevation, and creation/update.
 * * If the rule is missing, this function relaunches the application with admin privileges,
 * then relaunches the final instance with original arguments. The process then exits.
 * * This overload retrieves the command line arguments internally.
 * @return int The exit code for the current process instance.
 * Returns -1 if the application should proceed normally.
 * Returns 0 or 1 if the process should exit immediately.
 */
int EnsureFirewallRule(void);

#endif // _WIN32