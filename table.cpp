// Teal Dulcet

// Requires downloading the Table header only library: https://github.com/tdulcet/Table-and-Graph-Libs/blob/master/tables.hpp

// Compile: g++ -std=gnu++17 -Wall -g -O3 -flto table.cpp -o table

// Run: ./table [OPTION(S)]... [FILE(S)]...

#include <fstream>
#include <climits>
#include <getopt.h>

#include "tables.hpp"

using namespace std;

enum
{
	GETOPT_HELP_CHAR = CHAR_MIN - 2,
	GETOPT_VERSION_CHAR = CHAR_MIN - 3
};

const char *const style_args[] = {"ascii", "basic", "light", "heavy", "double", "arc", "light-dashed", "heavy-dashed"};

// Check if the argument is in the argument list
template <typename T>
T xargmatch(const char *const context, const char *const arg, const char *const *arglist, const size_t argsize, const T vallist[])
{
	const size_t arglen = strlen(arg);

	for (size_t i = 0; i < argsize; ++i)
		if (!strncmp(arglist[i], arg, arglen) and strlen(arglist[i]) == arglen)
			return vallist[i];

	cerr << "Error: Invalid argument " << quoted(arg) << " for " << quoted(context) << '\n';
	exit(1);

	// return -1;
}

template <typename T>
vector<basic_string<T>> split(const basic_string<T> &s, const T delim = ',')
{
	basic_istringstream<T> ss(s);
	basic_string<T> item;
	vector<basic_string<T>> result;

	while (getline(ss, item, delim))
	{
		result.push_back(item);
	}

	return result;
}

template <typename T>
vector<vector<basic_string<T>>> input(basic_istream<T> &in, const char *delimiter, const char line_delim, const bool keep_empty_lines)
{
	vector<vector<basic_string<T>>> aarray;

	basic_string<T> line;
	while (getline(in, line, line_delim))
	{
		if (!line.empty())
		{
			vector<basic_string<T>> array;

			if (delimiter)
			{
				size_t pos = 0;
				do
				{
					const size_t end = line.find_first_of(delimiter, pos);
					array.push_back(line.substr(pos, end - pos));
					pos = end != basic_string<T>::npos ? end + 1 : end;
				} while (pos != basic_string<T>::npos);
			}
			else
			{
				basic_istringstream<T> ss(line);
				basic_string<T> token;
				while (ss >> token)
				{
					array.push_back(token);
				}
			}

			aarray.push_back(array);
		}
		else if (keep_empty_lines)
			aarray.push_back({});
	}

	return aarray;
}

// Output usage
void usage(const char *const programname)
{
	cerr << "Usage:  " << programname << R"( [OPTION(S)]... [FILE(S)]...
or:     )"
		 << programname << R"d( <OPTION>
Convert input into a table. With no FILE, or when FILE is -, read from standard input. Empty lines are ignored. All rows should have the same number of columns. Table cells can contain Unicode characters and formatted text with ANSI escape sequences. See examples below.

Options:
    Mandatory arguments to long options are mandatory for short options too.
    -t, --title <TITLE>     Table Name/Title
    -n, --name <TITLE>          Show a title above the table. The title is word wrapped based on the current width of the terminal.
    -N, --columns <NAMES>   Column names
                                Adds row to top of the table. Provide either a comma separated list of names or specify this option multiple times, once for each column.
    -M, --rows <NAMES>      Row names
                                Adds column to left of the table. Provide either a comma separated list of names or specify this option multiple times, once for each row.
    -l, --left              Left align (default)
    -R, --right             Right align
    -r, --header-row        Header row
                                Header rows are bolded, centered and have a border.
    -c, --header-column     Header column
                                Header columns are bolded, centered and have a border.
    -b, --no-border         No table border
    -C, --cell-border       Show cell border
    -L, --keep-empty-lines  Do not ignore empty lines
    -s, --separator <SEP>   Characters to delimit columns/fields (default any whitespace)
    -d, --delimiter <SEP>   
    -z, --zero-terminated   Line delimiter is NUL, not newline
    -p, --padding <PADDING> Cell padding (default 1)
    -S, --style <STYLE>     Border style (default 'light')
                                <STYLE> can be:
                                    ascii:          ASCII
                                    basic:          Basic
                                    light:          Light (default)
                                    heavy:          Heavy
                                    double:         Double
                                    arc:            Light Arc
                                    light-dashed:   Light Dashed
                                    heavy-dashed:   Heavy Dashed

        --help              Display this help and exit
        --version           Output version information and exit

Examples:
    Output table
    $ printf 'a b c\n1 2 3\n' | )d"
		 << programname << R"(

    Output table with separator/delimiter
    $ printf 'a:b:c\n1::3\n' | )"
		 << programname << R"( --separator ':'

    Output table with header row
    $ ls -l --color | tail -n +2 | )"
		 << programname << R"d( --header-row --columns 'PERM,LINKS,OWNER,GROUP,SIZE,MONTH,DAY,HH:MM/YEAR,NAME'

    Output table with header row and column (Bash syntax)
    $ printf '%s\t%s\t%s\t%s\n' 'Data '{1..16} | )d"
		 << programname << R"d( --separator $'\t' --header-row --header-column --columns 'Header row/column 1' --columns='Header row '{2..5} --rows='Header column '{2..5}

    Output table of values (Bash syntax)
    $ for i in {-10..10}; do echo "$i $(( i + 1 ))"; done | )d"
		 << programname << R"d( --header-row --columns 'x,y'

    Output sorted table (Bash syntax)
    $ for i in {0..4}; do for j in {0..4}; do echo -n "$(( RANDOM * RANDOM )) "; done; echo; done | sort -n -k 1 | )d"
		 << programname << R"d(

    Output a table in each style (Bash syntax)
    $ for s in ascii basic light heavy double arc light-dashed heavy-dashed; do printf 'a b c\n1 2 3\n' | )d"
		 << programname << R"( --cell-border --style=$s --title "Style: $s"; done

)";
}

int main(int argc, char *argv[])
{
	vector<string> aheaderrow;
	vector<string> aheadercolumn;

	tables::options aoptions;
	aoptions.check = false;

	const char *delimiter = nullptr;
	char line_delim = '\n';
	
	bool keep_empty_lines = false;

	const int frombase = 0;

	// https://stackoverflow.com/a/38646489

	static struct option long_options[] = {
		{"title", required_argument, nullptr, 't'},
		{"name", required_argument, nullptr, 'n'},
		{"columns", required_argument, nullptr, 'N'},
		{"rows", required_argument, nullptr, 'M'},
		// {"header-repeat", no_argument, NULL, 'e'},
		{"left", no_argument, nullptr, 'l'},
		{"right", no_argument, nullptr, 'R'},
		// {"truncate", no_argument, NULL, 'T'},
		// {"wrap", no_argument, NULL, 'W'},
		{"separator", required_argument, nullptr, 's'},
		{"delimiter", required_argument, nullptr, 'd'},
		{"keep-empty-lines", no_argument, nullptr, 'L'},
		{"zero-terminated", no_argument, nullptr, 'z'},
		{"header-row", no_argument, nullptr, 'r'},
		{"header-column", no_argument, nullptr, 'c'},
		{"no-border", no_argument, nullptr, 'b'},
		{"cell-border", no_argument, nullptr, 'C'},
		{"padding", required_argument, nullptr, 'p'},
		{"style", required_argument, nullptr, 'S'},
		{"help", no_argument, nullptr, GETOPT_HELP_CHAR},
		{"version", no_argument, nullptr, GETOPT_VERSION_CHAR},
		{nullptr, 0, nullptr, 0}};

	int option_index = 0;
	int c = 0;

	while ((c = getopt_long(argc, argv, "bcd:lrn:p:s:t:zCLRS:M:N:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'b':
			aoptions.tableborder = false;
			break;
		case 'c':
			aoptions.headercolumn = true;
			break;
		case 'l':
			aoptions.alignment = ios_base::left;
			break;
		case 'r':
			aoptions.headerrow = true;
			break;
		case 'p':
			aoptions.padding = strtol(optarg, nullptr, frombase);
			break;
		case 's':
		case 'd':
			delimiter = optarg;
			break;
		case 't':
		case 'n':
			aoptions.title = optarg;
			break;
		case 'C':
			aoptions.cellborder = true;
			break;
		case 'L':
			keep_empty_lines = true;
			break;
		case 'R':
			aoptions.alignment = ios_base::right;
			break;
		case 'S':
			aoptions.style = xargmatch("--style", optarg, style_args, size(style_args), tables::style_types);
			break;
		case 'M':
			aheadercolumn.emplace_back(optarg);
			break;
		case 'N':
			aheaderrow.emplace_back(optarg);
			break;
		case 'z':
			line_delim = '\0';
			break;
		case GETOPT_HELP_CHAR:
			usage(argv[0]);
			return 0;
		case GETOPT_VERSION_CHAR:
			cout << "Table 1.0\n\n";
			return 0;
		case '?':
			cerr << "Try '" << argv[0] << " --help' for more information.\n";
			return 1;
		default:
			abort();
		}
	}

	vector<vector<string>> aarray;

	if (optind < argc)
	{
		for (int i = optind; i < argc; ++i)
		{
			vector<vector<string>> aaarray;

			if (string(argv[i]) == "-")
			{
				aaarray = input(cin, delimiter, line_delim, keep_empty_lines);

				aarray.insert(aarray.end(), aaarray.cbegin(), aaarray.cend());
			}
			else
			{
				ifstream fin(argv[i]);

				if (fin)
				{
					aaarray = input(fin, delimiter, line_delim, keep_empty_lines);

					aarray.insert(aarray.end(), aaarray.cbegin(), aaarray.cend());

					// fin.close();
				}
				else
					cerr << "Error: Unable to open the " << quoted(argv[i]) << " file (" << strerror(errno) << ").\n";
			}
		}
	}
	else
	{
		aarray = input(cin, delimiter, line_delim, keep_empty_lines);
	}

	if (aarray.empty())
		return 0;

	const size_t max = (*max_element(aarray.cbegin(), aarray.cend(), [](const auto &a, const auto &b)
									 { return a.size() < b.size(); }))
						   .size();

	for (auto &array : aarray)
	{
		if (array.size() != max)
		{
			if (!array.empty() or !keep_empty_lines)
				cerr << "Warning: The rows of the array should have the same number of columns (" << max << ").\n";
			array.resize(max);
		}
	}

	size_t rows = aarray.size();
	size_t columns = aarray[0].size();

	if (aheaderrow.data())
		++rows;

	if (aheadercolumn.data())
		++columns;

	if (aheaderrow.data())
	{
		if (aheaderrow.size() == 1 and columns != 1)
			aheaderrow = split(aheaderrow[0]);

		if (aheaderrow.size() != columns)
		{
			cerr << "Warning: The header row does not have the same number of columns (" << aheaderrow.size() << ") as the array (" << columns << ").\n";
			aheaderrow.resize(columns);
		}
	}

	if (aheadercolumn.data())
	{
		if (aheadercolumn.size() == 1 and rows != 1)
			aheadercolumn = split(aheadercolumn[0]);

		const size_t asize = aheaderrow.data() ? rows - 1 : rows;
		if (aheadercolumn.size() != asize)
		{
			cerr << "Warning: The header column does not have the same number of rows (" << aheadercolumn.size() << ") as the array (" << asize << ").\n";
			aheadercolumn.resize(asize);
		}
	}

	return tables::array(aarray, aheaderrow.data(), aheadercolumn.data(), aoptions);
}
