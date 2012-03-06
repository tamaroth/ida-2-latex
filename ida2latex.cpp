#include <ida.hpp>
#include <idp.hpp>
#include <expr.hpp>
#include <name.hpp>
#include <area.hpp>
#include <funcs.hpp>
#include <bytes.hpp>
#include <diskio.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

#define MAX_LINE_SIZE 1024

char lt_head[] = 
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

char lt_foot[] =
	"}\n"
	"\\end{document}\n";

char lt_default[] = "";
char lt_grey[] = "\\textcolor{Grey}{";
char lt_navy[] = "\\textcolor{Navy}{";
char lt_blue[] = "\\textcolor{Blue}{";
char lt_green[] = "\\textcolor{Green}{";
char lt_lblue[] = "\\textcolor{LBlue}{";
char lt_lgreen[] = "\\textcolor{LGreen}{";
char lt_purple[] = "\\textcolor{Purple}{";
char lt_pink[] = "\\textcolor{Pink}{";
char lt_red[] = "\\textcolor{Red}{";
char lt_orange[] = "\\textcolor{Orange}{";
char lt_gold[] = "\\textcolor{Gold}{";

char st_uparrow[] = {0x19,0};
char st_downarrow[] = {0x18,0};

#define NXT_ESC_CODE is_colour_code = false; \
					 is_escape_code = true;

#define NXT_CLR_CODE is_colour_code = true; \
					 is_escape_code = false;

char *prefixes[] = {
	lt_default,
	lt_blue,
	lt_grey,
	lt_navy,
	lt_green,
	lt_lblue,
	lt_gold,
	lt_red,
	lt_lgreen,
	lt_purple,
	lt_pink
};

size_t prefix_sizes[] = {
	1,
	sizeof(lt_blue),
	sizeof(lt_grey),
	sizeof(lt_navy),
	sizeof(lt_green),
	sizeof(lt_lblue),
	sizeof(lt_gold),
	sizeof(lt_red),
	sizeof(lt_lgreen),
	sizeof(lt_purple),
	sizeof(lt_pink)
};

enum {
	BLACK = 0,
	BLUE,
	GREY,
	NAVY,
	GREEN,
	LBLUE,
	GOLD,
	RED,
	LGREEN,
	PURPLE,
	PINK,
	MAX_COLOUR
};

char *findAndReplace( char *src, char *what, char *to, size_t len )
{
	char *dst,*p,*q,*tmp;
	int c = 0;
		
	// first we need to find out how many characters we need to replace
	p = src;
	tmp = strstr( p, what );

	while( tmp != NULL )
	{
		c++;
		p += ( ( tmp-p ) + strlen( what ) );
		tmp = strstr( p, what );
	}

	// if no characters are found, return the source
	if ( !c )
	{
		return src;
	}

	// calculate new length of a line
	len = len - ( c * strlen(what) );
	len = len + ( c * strlen(to) );

	// allocate and zero new memory
	dst = new char[len];
    if ( dst == NULL )
    {
        msg("Memory allocation failure.");
        return src;
    }
	memset( dst, 0, len) ;

	// replace all characters that we are looking for
	q = dst;
	p = src;
	tmp = strstr( p, what );
	while ( tmp != NULL )
	{
		memcpy( q, p, ( tmp - p ) );
		strcat( q, to );
		q += strlen( q );
		p += ( tmp - p );
		p += strlen( what );
		tmp = strstr( p, what );
		c++;
	}
	
	// copy the remaining characters and remove the source
	memcpy( q, p, strlen( p ) );
	delete [] src;
	return dst;
}

void painttext( char *&src, char *&dst, int &prev_colour, int color, bool paint )
{
	if ( !paint )
		return;

	if( prev_colour != color )
	{
		memcpy( dst, prefixes[color], prefix_sizes[color] );
		dst += ( prefix_sizes[color] - 1 );
	}
	else
	{
		dst--;
	}

	// text to pain lasts till finishing escape character \02 or NULL termination
	size_t res = 0;
	const char *p = src;
	while ( *p != 0 && *p != 2 )
	{
		res++;
		p++;
	}

	char *tmp = new char[res+1];
    if ( tmp == NULL )
    {
        msg("Memory allocation failure.");
        return;
    }
	memset(tmp, 0, res+1);
	memcpy(tmp, src, res);
	src += res;

    // replace all unprintable characters, the order is important
	tmp = findAndReplace(tmp, "$", "\\$", strlen(tmp));
	tmp = findAndReplace(tmp, "\\", "$\\backslash$", res);?
	tmp = findAndReplace(tmp, "{", "\\{", strlen(tmp));
	tmp = findAndReplace(tmp, "}", "\\}", strlen(tmp));
	tmp = findAndReplace(tmp, "_", "\\_", strlen(tmp));
	tmp = findAndReplace(tmp, "%", "\\%", strlen(tmp));
	tmp = findAndReplace(tmp, "#", "\\#", strlen(tmp));
	tmp = findAndReplace(tmp, "&", "\\&", strlen(tmp));
	tmp = findAndReplace(tmp, "^", "\\verb1^1", strlen(tmp));
	tmp = findAndReplace(tmp, "~", "\\verb1~1", strlen(tmp));
	tmp = findAndReplace(tmp, st_uparrow, "$\\uparrow$", strlen(tmp));
	tmp = findAndReplace(tmp, st_downarrow, "$\\downarrow$", strlen(tmp));
	res = strlen(tmp);

	for(unsigned int i=0; i<res; i++)
	{
		if(tmp[i] == 01 && tmp[i+1] ==0x28)
		{
			memmove(tmp+i,tmp+i+10,res-i-10);
			tmp[res-10] = '\0';
			res = res-10;
		}
	}

	memcpy(dst, tmp, res);
	dst += res;
	dst[0] = '}';
	dst++;
	prev_colour = color;
	delete [] tmp;
	return;
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
	ea_t saddr,eaddr;
	ea_t addr;
	int uac=0,line_num=0;

	saddr = get_screen_ea();
	if (!isEnabled(saddr))
	{
		return;
	}
	int selection = read_selection(&saddr, &eaddr);
	if(!selection)
	{
		func_t *func = get_func(get_screen_ea());
		if(func == NULL)
		{
			msg("Plugin only supports code blocks... Sorry!\n");
			return;
		}

		saddr = func->startEA;
		eaddr = func->endEA;
	}

	// count number of lines selected
	for (addr = saddr; addr <= eaddr;)
	{
		uac = ua_code(addr);
		if (uac)
		{
			line_num++;
		} else {
			break;
		}
		addr += uac;
	}

	if(uac)
		line_num--;

	addr = saddr;
	char *fname = askfile_c(1, "*.tex", "Where to save the file?");
	if(fname==NULL)
	{
		msg("No file selected.. Aborting\n");
		return;
	}
	FILE *fp = fopenA(fname);

	ewrite(fp, lt_head, sizeof(lt_head)-1);
	while (line_num--)
	{
		jumpto(addr);
		int flags = calc_default_idaplace_flags();
		linearray_t ln(&flags);
		idaplace_t pl;
		pl.ea = addr;
		pl.lnnum = 0;
		ln.set_place(&pl);
		int n = ln.get_linecnt();           // how many lines for this address?

		for (int i=0; i< n; i++)
		{
			// process line
			char *q = new char[MAX_LINE_SIZE];
            if ( q == NULL )
            {
                msg("Memory allocation failure.");
                return;
            }
			memset(q, 0, MAX_LINE_SIZE);
			char *nline = q;
			char *line = ln.down();           // get line
			bool paint = false;
			bool is_colour_code = false;	// at the start of a line, first character must be an escape char followed by color code
			bool is_escape_code = true;

			int prev_colour = -1;			// set the colour to NULL;
			int paint_colour = BLACK;
			int escape_code = -1;
			while (tag_strlen(line))
			{
				switch(line[0])
				{
				case 1:	// this can be either escape character or color code
					line++;
					if(is_colour_code && escape_code == 1)	// if the escape code was 1, we get to paint now
					{
						painttext(line, nline, prev_colour, BLACK, true);
						nline--;
						nline[0] = '\0';
						paint = false;
						NXT_ESC_CODE
					} else if(is_escape_code)			// this escape code starts the text, next character must be colour code
					{
						NXT_CLR_CODE
						escape_code = 1;
					} else
					{
						line++;
						NXT_ESC_CODE
						escape_code = 2;
					}
					break;
	
				case 2:
					line++;
					if(is_colour_code && escape_code == 1)		// paint regular comment - blue
					{
						paint_colour = BLUE;
						NXT_ESC_CODE
						if(line[0]==1)
							line += 10;
						paint = true;
					} else if (is_escape_code)
					{
						NXT_CLR_CODE
						escape_code = 2;
					} else
					{
						line++;
						NXT_ESC_CODE
						escape_code = 2;
					}
					break;

				case 3:			// grey
					line++;
					if(is_escape_code)
					{
						paint_colour = GREY;
						NXT_ESC_CODE;
						break;
					}
				case 4:
					line++;
					if(is_escape_code)
					{
						line++;
						NXT_ESC_CODE
						break;
					}
					if(escape_code == 1)
					{
						paint_colour = GREY;
						if(line[0]==1)
							line +=10;
						paint = true;
					}
					NXT_ESC_CODE
					break;
	
				case 5:			// navy
				case 6:
				case 9:
				case 0x21:
				case 0x24:
				case 0x26:
					line++;
					if(escape_code == 1)
					{
						paint_colour = NAVY;
						if(line[0]==1)
							line += 10;
						paint = true;
					}
					NXT_ESC_CODE
					break;
	
				case 0x0A:		// green
				case 0x0B:
				case 0x0C:
				case 0x0E:
				case 0x19:
				case 0x1E:
					line++;
					if(escape_code == 1)
					{
						paint_colour = GREEN;
						if(line[0]==1)
							line += 10;
						paint = true;
					}
					NXT_ESC_CODE
					break;
	
				case 7:			// blue
				case 8:
				case 0x14:
				case 0x15:
				case 0x16:
				case 0x1A:
				case 0x1B:
				case 0x25:
				case 0x27:
					line++;
					if(escape_code==1)
					{
						paint_colour = BLUE;
						if(line[0]==1)
							line += 10;
						paint = true;
					}
					NXT_ESC_CODE
					break;
	
				case 0x0F:		// light blue
				case 0x18:
					line++;
					if(escape_code==1)
					{
						paint_colour = LBLUE;
						if(line[0]==1)
							line += 10;
						paint = true;
					} 
					NXT_ESC_CODE
					break;
	
				case 0x10:		// gold
				case 0x11:
				case 0x23:
					line++;
					if(escape_code==1)
					{
						paint_colour = GOLD;
						if(line[0]==1)
							line += 10;
						paint = true;
					} 
					NXT_ESC_CODE
					break;			

				case 0x12:		// red
					line++;
					if(escape_code==1)
					{
						paint_colour = RED;
						if(line[0]==1)
							line += 10;
						paint = true;
					} 
					NXT_ESC_CODE
					break;		

				case 0x1C:		// purple
					line++;
					if(escape_code==1)
					{
						paint_colour = PURPLE;
						if(line[0]==1)
							line += 10;
						paint = true;
					} 
					NXT_ESC_CODE
					break;

				case 0x17:
					line++;
					if(escape_code==1)
					{
						paint_colour = GREY;
						if(line[0]==1 && line[1]==0x28)
							line+=10;
						paint = true;
					}
					NXT_ESC_CODE
					break;

				case 0x1F:		// light green
					line++;
					if(escape_code==1)
					{
						paint_colour = LGREEN;
						if(line[0]==1)
							line += 10;
						paint = true;
						break;
					}
					NXT_ESC_CODE
					break;

				case 0x22:		// pink
					line++;
					if(escape_code==1)
					{
						paint_colour = PINK;
						if(line[0]==1)
							line += 10;
						paint = true;
					} 
					NXT_ESC_CODE
					break;
	
				case 0x0D:		// black
				case 0x13:
					line++;
					if(is_colour_code && escape_code == 1)
					{
						if(line[0]==1)
							line += 10;
						painttext(line, nline, prev_colour, BLACK, true);
						nline--;
						nline[0] = '\0';
					}
					NXT_ESC_CODE
					break;	

				case 0x20:		// space
					line++;
					if(is_colour_code && escape_code == 1)
					{
						// paint it navy..
						paint_colour = NAVY;
						if(line[0]==1)
							line += 10;
						paint = true;
					} else if (is_escape_code)
					{
						nline[0] = '~';
						nline++;
						prev_colour = BLACK;
					}
					NXT_ESC_CODE
					break;	
	
				case 0x28:
				case 0x29:
				case 0x2A:
					line++;
					NXT_ESC_CODE
					break;

				default:		// just print the character
					nline[0] = line[0];
					nline++;
					line++;
					NXT_ESC_CODE
					break;
				}
				painttext(line, nline, prev_colour, paint_colour, paint);
				paint = false;
			}
	
			strcpy(nline, "\\linebreak\n");
			ewrite(fp, q, (ssize_t)strlen(q));
			delete [] q;
		}
		addr = addr + ua_code(addr);
	}
	ewrite(fp, lt_foot, sizeof(lt_foot)-1);
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
