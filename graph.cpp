// Teal Dulcet

// Requires downloading the Table header only library: https://github.com/tdulcet/Table-and-Graph-Libs/blob/master/tables.hpp
// Requires downloading the Graph header only library: https://github.com/tdulcet/Table-and-Graph-Libs/blob/master/graphs.hpp

// Compile: g++ -std=gnu++17 -Wall -g -O3 -flto graph.cpp -o graph

// Run: ./graph [OPTION(S)]... [FILE(S)]...

#include <fstream>
#include <climits>
#include <cinttypes>
#include <getopt.h>

#include "tables.hpp"
#include "graphs.hpp"

using namespace std;

enum
{
	X_UNITS_OPTION = CHAR_MAX + 1,
	Y_UNITS_OPTION,
	GETOPT_HELP_CHAR = CHAR_MIN - 2,
	GETOPT_VERSION_CHAR = CHAR_MIN - 3
};

const char *const style_args[] = {"ascii", "basic", "light", "heavy", "double", "arc", "light-dashed", "heavy-dashed"};

const char *const color_args[] = {"default", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "gray", "bright-red", "bright-green", "bright-yellow", "bright-blue", "bright-magenta", "bright-cyan", "bright-white"};

const char *const type_args[] = {"braille", "block", "block-quadrant", "separated-block-quadrant", "block-sextant", "separated-block-sextant", "block-octant" /* , "histogram" */};

const char *const mark_args[] = {"dot", "plus", "square"};

const char *const units_args[] = {"number", "si", "iec", "iec-i", "fracts", "percent", "date", "time", "monetary"};

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
int column(const size_t width, const T &array, const tables::options &aoptions)
{
	const size_t rows = array.size();
	const size_t columns = array[0].size();

	vector<vector<int>> rowwidth(rows, vector<int>(columns));

	for (size_t i = 0; i < rows; ++i)
	{
		for (size_t j = 0; j < columns; ++j)
			rowwidth[i][j] = tables::strcol(array[i][j].c_str());
	}

	const size_t total = rows * columns;
	size_t acolumns = total;

	for (; acolumns > columns; acolumns -= columns)
	{
		vector<int> columnwidth(acolumns);

		for (size_t i = 0; i < rows; ++i)
		{
			const size_t k = (i * columns) % acolumns;

			for (size_t j = 0; j < columns; ++j)
			{
				if (rowwidth[i][j] > columnwidth[k + j])
					columnwidth[k + j] = rowwidth[i][j];
			}
		}

		size_t awidth = accumulate(columnwidth.cbegin(), columnwidth.cend(), 0ul);

		if (aoptions.tableborder or aoptions.cellborder or aoptions.headerrow or aoptions.headercolumn)
			awidth += (((2 * aoptions.padding) + 1) * acolumns) + (aoptions.tableborder ? 1 : -1);
		else
			awidth += (2 * aoptions.padding) * acolumns;

		if (awidth <= width)
			break;
	}

	const size_t arows = (total + acolumns - 1) / acolumns;

	vector<vector<string>> aarray(arows);

	for (size_t i = 0; i < rows; ++i)
	{
		const size_t k = (i * columns) / acolumns;

		aarray[k].insert(aarray[k].end(), array[i].cbegin(), array[i].cend());
	}

	if (total % acolumns)
		aarray.back().resize(acolumns);

	string *headerrow = nullptr;
	string *headercolumn = nullptr;

	return tables::array(aarray, headerrow, headercolumn, aoptions);
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
vector<vector<basic_string<T>>> input(basic_istream<T> &in, const char *delimiter, const char line_delim)
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
	}

	return aarray;
}

// Output usage
void usage(const char *const programname)
{
	cerr << "Usage:  " << programname << R"( [OPTION(S)]... [FILE(S)]...
or:     )"
		 << programname << R"d( <OPTION>
Convert input into a graph/plot. With no FILE, or when FILE is -, read from standard input. Empty lines are ignored. With a single input, each row can have one or more columns in the form 'x y1 ... yn', which will be converted to 'x y1' ... 'x yn'. With a single input and column it will output a histogram, otherwise it will output a plot. With multiple inputs, rows should have exactly two columns in the form 'x y'. See examples below.

Options:
    Mandatory arguments to long options are mandatory for short options too.
    -t, --title <TITLE>     Graph Name/Title
        --name <TITLE>          Show a title above the graph. The title is word wrapped based on the current width of the terminal.
    -h, --height <HEIGHT>   Graph height (default 0)
                                If HEIGHT is 0, it will be set to the current height of the terminal (number of rows).
    -w, --width <WIDTH>     Graph width (default 0)
                                If WIDTH is 0, it will be set to the current width of the terminal (number of columns).
    -x, --x-min <XMIN>      Minimum x value (default 0)
                                If XMIN and XMAX are both 0, they will be set to the respective minimum and maximum values of x in the input.
    -X, --x-max <XMAX>      Maximum x value (default 0)
                                If XMIN and XMAX are both 0, they will be set to the respective minimum and maximum values of x in the input.
    -y, --y-min <YMIN>      Minimum y value (default 0)
                                If YMIN and YMAX are both 0, they will be set to the respective minimum and maximum values of y in the input.
    -Y, --y-max <YMAX>      Maximum y value (default 0)
                                If YMIN and YMAX are both 0, they will be set to the respective minimum and maximum values of y in the input.
    -C, --type <TYPE>       Type (default 'braille')
                                <TYPE> can be:
                                    braille:                  Braille (default)
                                    block:                    Block
                                    block-quadrant:           Block quadrant
                                    separated-block-quadrant: Separated block quadrant
                                    block-sextant:            Block sextant
                                    separated-block-sextant:  Separated block sextant
                                    block-octant:             Block octant
    -m, --mark <MARK>       Mark type (default 'dot')
                                <MARK> can be:
                                    dot:            Dot (default)
                                    plus:           Plus
                                    square:         Square
    -b, --border            Border
                                Show graph and legend border.
    -a, --no-axis           No axis
                                Do not show the axis. Implies --no-axis-labels, --no-ticks and --no-units-labels.
    -l, --no-axis-labels    No axis labels
    -T, --no-ticks          No axis tick marks
                                Implies --no-units-labels.
    -u, --no-units-labels   No axis units labels
        --x-units <UNIT>    X-axis units format (default 'fracts')
                                See UNIT below.
        --y-units <UNIT>    Y-axis units format (default 'fracts')
                                See UNIT below.
    -n, --names <NAMES>     Series names
                                Provide either a comma separated list of names or specify this option multiple times, once for each input/series.
    -L, --legend            Legend
                                Show a legend below the graph. Uses the --names values or the first line/row of each input if that options is not provided.
    -s, --separator <SEP>   Characters to delimit columns/fields (default any whitespace)
    -d, --delimiter <SEP>   
    -z, --zero-terminated   Line delimiter is NUL, not newline
    -i, --int               Integer numbers
                                Read input values as integer numbers. Supports all Integer numbers )d"
		 << INTMAX_MIN << " - " << INTMAX_MAX << R"d(.
    -f, --float             Floating point numbers (default)
                                Read input values as floating point numbers. Supports all Floating point numbers )d"
		 << LDBL_MIN << " - " << LDBL_MAX << R"d(.
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
    -c, --color <COLOR>     Graph/Plot color (default 'red')
                                Used only when plotting a single input/series. Otherwise, colors red - bright white (2 - 16) are used inorder. The system default color is used where the plots cross. <COLOR> can be:
                                    default:        System default
                                    black:          Black
                                    red:            Red (default)
                                    green:          Green
                                    yellow:         Yellow
                                    blue:           Blue
                                    magenta:        Magenta
                                    cyan:           Cyan
                                    white:          White
                                    gray:           Gray
                                    bright-red:     Bright Red
                                    bright-green:   Bright Green
                                    bright-yellow:  Bright Yellow
                                    bright-blue:    Bright Blue
                                    bright-magenta: Bright Magenta
                                    bright-cyan:    Bright Cyan
                                    bright-white:   Bright White

        --help              Display this help and exit
        --version           Output version information and exit

UNIT options:
    number:         Locale number format (similar to 'numfmt --grouping')
                        e.g. 1234.25 → 1,234.25
    si:             Auto-scale to the SI standard (similar to 'numfmt --to=si')
                        e.g. 1000 → 1K
    iec:            Auto-scale to the IEC standard (similar to 'numfmt --to=iec')
                        e.g. 1024 → 1K
    iec-i:          Auto-scale to the IEC standard (similar to 'numfmt --to=iec-i')
                        e.g. 1024 → 1Ki
    fracts:         Locale number format, but convert fractions and mathematical constants to Unicode characters (default)
                        e.g. 1234.25 → 1,234¼
    percent:        Percentage format
                        e.g. 0.123 → 12.3%
    date:           Locale date format (same as 'date +%x')
                        e.g. 2147483647 → 01/19/2038
    time:           Locale time format (same as 'date +%X')
                        e.g. 2147483647 → 03:14:07 AM
    monetary:       Locale monetary/currency format (does not work with the ‘C’ locale)
                        e.g. 123 → $1.23

Examples:
    Output plot
    $ printf '1 1\n2 2\n3 3\n4 4\n5 5\n6 6\n' | )d"
		 << programname << R"d( --height 20 --width 40 --x-min -10 --x-max 10 --y-min -10 --y-max 10

    Output plot of single series (Bash Syntax)
    $ for i in {-20..20}; do echo "$i $(( i + 1 ))"; done | )d"
		 << programname << R"d( --height 40 --width 80 --x-min -20 --x-max 20 --y-min -20 --y-max 20

    Output plot of multiple series (Bash Syntax)
    $ for i in {-20..20}; do echo "$i $(( 2 * i )) $(( i ** 2 ))"; done | )d"
		 << programname << R"d( --height 40 --width 80 --x-min -20 --x-max 20 --y-min -20 --y-max 20

    Output graph of multiple functions
    $ awk 'BEGIN { pi=atan2(0, -1); width=160; xmin=-(2*pi); xmax=2*pi; xstep=(xmax-xmin)/width; for(i=0; i<width*2; ++i) { x=((i/2)*xstep)+xmin; print x,sin(x),cos(x),sin(x)/cos(x) } }' | )d"
		 << programname << R"d( --height 40 --width 80 --y-min -4 --y-max 4 --no-units-labels

    Output a plot in each style (Bash syntax)
    $ for s in ascii basic light heavy double arc light-dashed heavy-dashed; do for i in {0..9}; do echo "$i $(( i + 1 ))"; done | )d"
		 << programname << R"( --height 20 --width 40 --x-min -10 --x-max 10 --y-min -10 --y-max 10 --style=$s --title "Style: $s"; done

)";
}

int main(int argc, char *argv[])
{
	size_t height = 0;
	size_t width = 0;

	long double xmin = -0;
	long double xmax = 0;
	long double ymin = -0;
	long double ymax = 0;

	graphs::options aoptions;
	aoptions.check = false;

	const char *delimiter = nullptr;
	char line_delim = '\n';

	vector<string> names;
	bool legend = false;

	bool integer = false;
	const int frombase = 0;
	char *p;

	setlocale(LC_ALL, "");

	// https://stackoverflow.com/a/38646489

	static struct option long_options[] = {
		{"title", required_argument, nullptr, 't'},
		{"name", required_argument, nullptr, 't'},
		{"separator", required_argument, nullptr, 's'},
		{"delimiter", required_argument, nullptr, 'd'},
		// {"keep-empty-lines", no_argument, NULL, 'L'},
		{"zero-terminated", no_argument, nullptr, 'z'},
		{"int", no_argument, nullptr, 'i'},
		{"float", no_argument, nullptr, 'f'},
		{"height", required_argument, nullptr, 'h'},
		{"width", required_argument, nullptr, 'w'},
		{"x-min", required_argument, nullptr, 'x'},
		{"x-max", required_argument, nullptr, 'X'},
		{"y-min", required_argument, nullptr, 'y'},
		{"y-max", required_argument, nullptr, 'Y'},
		{"type", required_argument, nullptr, 'C'},
		{"mark", required_argument, nullptr, 'm'},
		{"border", no_argument, nullptr, 'b'},
		{"no-axis", no_argument, nullptr, 'a'},
		{"no-axis-labels", no_argument, nullptr, 'l'},
		{"no-ticks", no_argument, nullptr, 'T'},
		{"no-units-labels", no_argument, nullptr, 'u'},
		{"x-units", required_argument, nullptr, X_UNITS_OPTION},
		{"y-units", required_argument, nullptr, Y_UNITS_OPTION},
		{"names", required_argument, nullptr, 'n'},
		{"legend", no_argument, nullptr, 'L'},
		{"style", required_argument, nullptr, 'S'},
		{"color", required_argument, nullptr, 'c'},
		{"help", no_argument, nullptr, GETOPT_HELP_CHAR},
		{"version", no_argument, nullptr, GETOPT_VERSION_CHAR},
		{nullptr, 0, nullptr, 0}};

	int option_index = 0;
	int c = 0;

	while ((c = getopt_long(argc, argv, "abc:d:fg:h:ilm:n:p:s:t:uw:x:y:zC:LS:TX:Y:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'a':
			aoptions.axis = false;
			break;
		case 'b':
			aoptions.border = true;
			break;
		case 'c':
			aoptions.color = xargmatch("--color", optarg, color_args, size(color_args), graphs::color_types);
			break;
		case 'f':
			integer = true;
			break;
		case 'h':
			height = strtoul(optarg, &p, frombase);
			if (*p)
			{
				cerr << "Usage: <HEIGHT> is not a valid integer number: " << quoted(optarg) << ".\n";
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Integer number for <HEIGHT> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ").\n";
				return 1;
			}
			break;
		case 'i':
			integer = false;
			break;
		case 'l':
			aoptions.axislabel = false;
			break;
		case 'm':
			aoptions.mark = xargmatch("--mark", optarg, mark_args, size(mark_args), graphs::mark_types);
			break;
		case 'n':
			names.emplace_back(optarg);
			legend = true;
			break;
		case 's':
		case 'd':
			delimiter = optarg;
			break;
		case 't':
			aoptions.title = optarg;
			break;
		case 'u':
			aoptions.axisunitslabel = false;
			break;
		case 'w':
			width = strtoul(optarg, &p, frombase);
			if (*p)
			{
				cerr << "Usage: <WIDTH> is not a valid integer number: " << quoted(optarg) << ".\n";
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Integer number for <WIDTH> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ").\n";
				return 1;
			}
			break;
		case 'x':
			xmin = strtold(optarg, &p);
			if (*p)
			{
				cerr << "Usage: <XMIN> is not a valid floating point number: " << quoted(optarg) << '\n';
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Floating point number for <XMIN> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ")\n";
				return 1;
			}
			break;
		case 'y':
			ymin = strtold(optarg, &p);
			if (*p)
			{
				cerr << "Usage: <YMIN> is not a valid floating point number: " << quoted(optarg) << '\n';
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Floating point number for <YMIN> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ")\n";
				return 1;
			}
			break;
		case 'z':
			line_delim = '\0';
			break;
		case 'C':
			aoptions.type = xargmatch("--type", optarg, type_args, size(type_args), graphs::type_types);
			break;
		case 'L':
			legend = true;
			break;
		case 'S':
			aoptions.style = xargmatch("--style", optarg, style_args, size(style_args), graphs::style_types);
			break;
		case 'T':
			aoptions.axistick = false;
			break;
		case 'X':
			xmax = strtold(optarg, &p);
			if (*p)
			{
				cerr << "Usage: <XMAX> is not a valid floating point number: " << quoted(optarg) << '\n';
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Floating point number for <XMAX> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ")\n";
				return 1;
			}
			break;
		case 'Y':
			ymax = strtold(optarg, &p);
			if (*p)
			{
				cerr << "Usage: <YMAX> is not a valid floating point number: " << quoted(optarg) << '\n';
				return 1;
			}
			if (errno == ERANGE)
			{
				cerr << "Error: Floating point number for <YMAX> is too large to input: " << quoted(optarg) << " (" << strerror(errno) << ")\n";
				return 1;
			}
			break;
		case X_UNITS_OPTION:
			aoptions.xunits = xargmatch("--x-units", optarg, units_args, size(units_args), graphs::units_types);
			break;
		case Y_UNITS_OPTION:
			aoptions.yunits = xargmatch("--y-units", optarg, units_args, size(units_args), graphs::units_types);
			break;
		case GETOPT_HELP_CHAR:
			usage(argv[0]);
			return 0;
		case GETOPT_VERSION_CHAR:
			cout << "Graph 1.0\n\n";
			return 0;
		case '?':
			cerr << "Try '" << argv[0] << " --help' for more information.\n";
			return 1;
		default:
			abort();
		}
	}

	vector<vector<vector<string>>> aaarray;

	if (optind < argc)
	{
		for (int i = optind; i < argc; ++i)
		{
			if (string(argv[i]) == "-")
			{
				aaarray.push_back(input(cin, delimiter, line_delim));
			}
			else
			{
				ifstream fin(argv[i]);

				if (fin)
				{
					aaarray.push_back(input(fin, delimiter, line_delim));

					// fin.close();
				}
				else
					cerr << "Error: Unable to open the " << quoted(argv[i]) << " file (" << strerror(errno) << ").\n";
			}
		}
	}
	else
	{
		aaarray.push_back(input(cin, delimiter, line_delim));
	}

	if (aaarray.empty() or (aaarray.size() == 1 and aaarray[0].empty()))
		return 0;

	if (aaarray.size() == 1)
	{
		const size_t max = (*max_element(aaarray[0].cbegin(), aaarray[0].cend(), [](const auto &a, const auto &b)
										 { return a.size() < b.size(); }))
							   .size();

		if (max > 2)
		{
			for (size_t i = 2; i < max; ++i)
			{
				vector<vector<string>> temp;
				temp.reserve(aaarray[0].size());

				for (const auto &array : aaarray[0])
				{
					if (array.size() > i)
					{
						temp.push_back({array[0], array[i]});
					}
				}

				aaarray.push_back(temp);
			}

			if (!all_of(aaarray[0].cbegin(), aaarray[0].cend(), [&max](const auto &array)
						{ return array.size() == max; }))
			{
				cerr << "Warning: The rows of the array should have the same number of columns (" << max << ").\n";
			}

			for (auto &array : aaarray[0])
			{
				if (array.size() != 2)
					array.resize(2);
			}
		}
	}

	size_t max = 0;
	for (auto &array : aaarray)
	{
		const size_t amax = (*max_element(array.cbegin(), array.cend(), [](const auto &a, const auto &b)
										  { return a.size() < b.size(); }))
								.size();
		if (amax > max)
			max = amax;
	}

	const size_t columns = max == 1 ? 1 : 2;

	if (!all_of(aaarray.cbegin(), aaarray.cend(), [&columns](const auto &array)
				{ return all_of(array.cbegin(), array.cend(), [columns](const auto &x)
								{ return x.size() == columns; }); }))
	{
		cerr << "Warning: The array should have one or two columns.\n";

		for (auto &array : aaarray)
		{
			for (auto &x : array)
			{
				if (x.size() != columns)
					x.resize(columns);
			}
		}
	}

	const size_t arrays = aaarray.size();

	if (columns == 1 and arrays != 1)
	{
		cerr << "Warning: Only one input/series supported for histograms (" << arrays << ").\n";
	}

	if (!names.empty())
	{
		if (names.size() == 1 and arrays != 1)
			names = split(names[0]);

		if (names.size() != arrays)
		{
			cerr << "Warning: There are not the same number of names (" << names.size() << ") as inputs/series (" << arrays << ").\n";
			names.resize(arrays);
		}
	}
	else if (legend)
	{
		for (auto &array : aaarray)
		{
			if (!array.empty())
			{
				names.push_back(array[0][1]);
				array.erase(array.begin());
			}
		}
	}

	auto ainput = [&]<typename T>() -> int
	{
		vector<vector<vector<T>>> aarray;
		aarray.reserve(arrays);

		for (const auto &array : aaarray)
		{
			vector<vector<T>> temp;
			temp.reserve(array.size());

			for (const auto &x : array)
			{
				vector<T> atemp;
				atemp.reserve(x.size());

				for (const auto &y : x)
				{
					const char *const token = y.c_str();
					T number;
					if constexpr (is_integral_v<T>)
					{
						char *p;
						number = strtoimax(token, &p, frombase);
						if (*p)
						{
							cerr << "Warning: Invalid integer number: " << quoted(token) << ".\n";
							return 1;
						}
						if (errno == ERANGE)
						{
							cerr << "Warning: Integer number too large to input: " << quoted(token) << " (" << strerror(errno) << ").\n";
							return 1;
						}
					}
					else
					{
						char *p;
						number = strtold(token, &p);
						if (*p)
						{
							cerr << "Warning: Invalid floating point number: " << quoted(token) << ".\n";
							return 1;
						}
						if (errno == ERANGE)
						{
							cerr << "Warning: Floating point number too large to input: " << quoted(token) << " (" << strerror(errno) << ").\n";
							return 1;
						}
					}
					atemp.push_back(number);
				}

				temp.push_back(atemp);
			}

			aarray.push_back(temp);
		}

		if (columns == 1)
		{
			vector<T> aaaarray;
			aaaarray.reserve(aarray[0].size());

			for (const auto &x : aarray[0])
				aaaarray.push_back(x[0]);

			return graphs::histogram(height, width, xmin, xmax, ymin, ymax, aaaarray, aoptions);
		}

		return graphs::plots(height, width, xmin, xmax, ymin, ymax, aarray, aoptions);
	};

	const int code = integer ? ainput.operator()<intmax_t>() : ainput.operator()<long double>();

	if (legend)
	{
		tables::options tableoptions;
		tableoptions.check = false;
		tableoptions.tableborder = aoptions.border;
		tableoptions.style = tables::style_types[aoptions.style];

		vector<array<string, 2>> aarray(arrays);

		for (size_t i = 0; i < arrays; ++i)
		{
			const unsigned acolor = arrays == 1 ? aoptions.color : (i % (size(graphs::colors) - 2)) + 2;
			aarray[i] = {graphs::outputcolor(graphs::color_type(acolor)) + string(columns == 1 ? graphs::bars[8] : aoptions.type == graphs::type_braille ? graphs::dots[255]
																																						 : graphs::blocks_quadrant[15]) +
							 graphs::outputcolor(graphs::color_default),
						 names[i]};
		}

		column((width / 2) + (aoptions.border ? 2 : 0), aarray, tableoptions);
	}

	return code;
}
