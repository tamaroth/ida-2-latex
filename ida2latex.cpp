#include <ida.hpp>
#include <idp.hpp>
#include <diskio.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

#include <string>
#include <map>

enum class COLOUR {
	BLK = 0,	// black
	BLU,		// blue
	GRY,		// gray
	NAV,		// navy
	GRN,		// green
	LBL,		// light blue
	GLD,		// gold
	RED,		// red
	LGR,		// light green
	PUR,		// purple
	PNK,		// pink
	MAX_COLOUR	// no colour
};

std::string prep(const std::string& input)
{
	std::string result("");
	char c_tmp[0x1000];
	if (tag_remove(input.c_str(), c_tmp, 0x1000) == -1)
		return result;

	std::string strippedLine(c_tmp);
	for (const auto& x : strippedLine)
	{
		switch (x)
		{
		case 0:
			return result;
		case '$':
			result += "\\$";
			break;
		case '\\':
			result += "$\\backslash$";
			break;
		case '{':
			result += "\\{";
			break;
		case '}':
			result += "\\}";
			break;
		case '_':
			result += "\\_";
			break;
		case '%':
			result += "\\%";
			break;
		case '#':
			result += "\\#";
			break;
		case '&':
			result += "\\&";
			break;
		case '^':
			result += "\\verb1^1";
			break;
		case '~':
			result += "\\verb1~1";
			break;
		case 0x18:
			result += "$\\downarrow$";
			break;
		case 0x19:
			result += "$\\uparrow$";
			break;
		case 0x20:
			result += "~";
			break;
		default:
			result += x;
		}
	}
	return result;
}

std::string paint(const std::string& src)
{
	static std::map<char, COLOUR> colour_tags = {
		{ 0x01, COLOUR::BLU },{ 0x02, COLOUR::BLU },{ 0x03, COLOUR::GRY },{ 0x04, COLOUR::GRY },
		{ 0x05, COLOUR::NAV },{ 0x06, COLOUR::NAV },{ 0x07, COLOUR::BLU },{ 0x08, COLOUR::BLU },
		{ 0x09, COLOUR::NAV },{ 0x0A, COLOUR::GRN },{ 0x0B, COLOUR::GRN },{ 0x0C, COLOUR::GRN },
		{ 0x0D, COLOUR::BLK },{ 0x0E, COLOUR::GRN },{ 0x0F, COLOUR::LBL },{ 0x10, COLOUR::GLD },
		{ 0x11, COLOUR::GLD },{ 0x12, COLOUR::RED },{ 0x13, COLOUR::BLK },{ 0x14, COLOUR::BLU },
		{ 0x15, COLOUR::BLU },{ 0x16, COLOUR::BLU },{ 0x17, COLOUR::GRY },{ 0x18, COLOUR::LBL },
		{ 0x19, COLOUR::GRN },{ 0x1A, COLOUR::BLU },{ 0x1B, COLOUR::BLU },{ 0x1C, COLOUR::PUR },
		{ 0x1D, COLOUR::BLU },{ 0x1E, COLOUR::GRN },{ 0x1F, COLOUR::LGR },{ 0x20, COLOUR::NAV },
		{ 0x21, COLOUR::BLU },{ 0x22, COLOUR::PNK },{ 0x23, COLOUR::GLD },{ 0x24, COLOUR::BLU },
		{ 0x25, COLOUR::BLU },{ 0x26, COLOUR::BLU },{ 0x27, COLOUR::BLU },{ 0x28, COLOUR::BLK },
	};

	std::map<COLOUR, std::string> prefixes = {
		{ COLOUR::BLK,	"" },
		{ COLOUR::BLU,	"\\textcolor{Blue}{" },
		{ COLOUR::GRY,	"\\textcolor{Grey}{" },
		{ COLOUR::NAV,	"\\textcolor{Navy}{" },
		{ COLOUR::GRN,	"\\textcolor{Green}{" },
		{ COLOUR::LBL,	"\\textcolor{LBlue}{" },
		{ COLOUR::GLD,	"\\textcolor{Gold}{" },
		{ COLOUR::RED,	"\\textcolor{Red}{" },
		{ COLOUR::LGR,	"\\textcolor{LGreen}{" },
		{ COLOUR::PUR,	"\\textcolor{Purple}{" },
		{ COLOUR::PNK,	"\\textcolor{Pink}{" }
	};

	std::string result;
	std::string block;
	COLOUR col = COLOUR::BLK;
	if (src.length() <= 0)
		return nullptr;

	for (size_t i = 0; i < src.length(); i++)
	{
		switch (src[i])
		{
		case 0x01:	///< Escape character (ON)

			if (src[i + 1] < 0x28)
			{
				col = colour_tags[src[i + 1]];
				result += prefixes[col];
			}
			block += src[i];
			continue;

		case 0x02:	///< Escape character (OFF)
			if (src[i + 1] >= 0x28)
			{
				block += src[i++];
				block += src[i];
				continue;
			}
			block += src[i++];
			block += src[i];

			result += prep(block);

			if (col != COLOUR::BLK)
				result += "}";
			block = "";
			continue;

		default:	///< Copy the remainder of the characters into the string.
			block += src[i];
			break;
		}
	}

	return result + "\\linebreak\n";
}


int idaapi init(void)
{
	msg("IDA2LaTeX loaded...\n");
	return PLUGIN_KEEP;
}

void idaapi term(void)
{
}

void idaapi run(int arg)
{
	(void)arg;
	ea_t saddr, eaddr;
	text_t disasm;
	std::string line;

	// Document header.
	const char lt_head[] =
		"\\documentclass[8pt]{article}\n"
		"\\usepackage{color}\n"
		"\\topmargin -15mm\n"
		"\\oddsidemargin -15mm\n"
		"\\evensidemargin -0.5in\n"
		"\\textwidth 190mm\n"
		"\\textheight 250mm\n"
		"\\definecolor{Grey}{RGB}{80,80,80}\n"
		"\\definecolor{Navy}{RGB}{0,0,128}\n"
		"\\definecolor{Blue}{RGB}{0,0,255}\n"
		"\\definecolor{Green}{RGB}{0,128,0}\n"
		"\\definecolor{LGreen}{RGB}{0,128,64}\n"
		"\\definecolor{LBlue}{RGB}{128,128,255}\n"
		"\\definecolor{Purple}{RGB}{128,0,128}\n"
		"\\definecolor{Pink}{RGB}{255,0,255}\n"
		"\\definecolor{Red}{RGB}{255,0,0}\n"
		"\\definecolor{Orange}{RGB}{255,128,0}\n"
		"\\definecolor{Gold}{RGB}{128,128,0}\n"
		"\\begin{document}\n"
		"\\section*{IDA code}\n"
		"\\texttt{\\linebreak\n";

	// Document footer.
	const char lt_foot[] =
		"}\n"
		"\\end{document}\n";

	saddr = get_screen_ea();
	if (!isEnabled(saddr))
	{
		return;
	}
	int selection = read_selection(&saddr, &eaddr);

	if (selection == 0)
	{
		msg("no selection...\n");
		func_t *func = get_func(get_screen_ea());
		if (func == NULL)
		{
			msg("Plugin only supports code blocks... Sorry!\n");
			return;
		}

		saddr = func->startEA;
		eaddr = func->endEA;
	}

	gen_disasm_text(saddr, eaddr, disasm, false);

	char *fname = askfile_c(1, "*.tex", "Where to save the file?");
	if (fname == NULL)
	{
		msg("No file selected.. Aborting\n");
		return;
	}
	FILE *fp = fopenWT(fname);
	ewrite(fp, lt_head, sizeof(lt_head) - 1);

	for (auto x : disasm)
	{
		line = paint(x.line);
		ewrite(fp, line.c_str(), line.length());
	}
	ewrite(fp, lt_foot, sizeof(lt_foot) - 1);
	eclose(fp);
	msg("Saved to %s...\n", fname);

}

char comment[] = "IDA 2 LaTeX!";

char help[] =
        "A plugin that converts selected code-area into a LaTeX file\n"
		"keeping coloring and format. It will use \\texttt{ branch\n"
        "for the code to display in."
        "\n"
        "I hope that someone will find it useful.\n";


//--------------------------------------------------------------------------
// This is the preferred name of the plugin module in the menu system
// The preferred name may be overriden in plugins.cfg file

char wanted_name[] = "IDA2LaTeX";


// This is the preferred hotkey for the plugin module
// The preferred hotkey may be overriden in plugins.cfg file
// Note: IDA won't tell you if the hotkey is not correct
//       It will just disable the hotkey.

char wanted_hotkey[] = "Ctrl-Alt-9";


//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_UNL,           // plugin flags
  init,                 // initialize
  term,                 // terminate. this pointer may be NULL.
  run,                  // invoke plugin
  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint
  help,                 // multiline help about the plugin
  wanted_name,          // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};
