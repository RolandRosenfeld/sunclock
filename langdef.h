#define N_MENU 28
#define N_ZOOM 13
#define N_OPTION 6
#define N_HELP 40

#define STDFORMATS "%H:%M%_%a%_%d%_%b%_%y|%H:%M:%S%_%Z|%a%_%j/%t%_%U/52"

enum {L_POINT=0, L_GMTTIME, L_SOLARTIME, L_LEGALTIME,
      L_DAYLENGTH, L_SUNRISE, L_SUNSET, 
      L_CLICKCITY, L_CLICKLOC, L_CLICK2LOC, L_DEGREE,
      L_OPTION, L_ACTIVATE, L_INCORRECT, L_KEY, L_CONTROLS, 
      L_ESCAPE, L_ESCMENU, L_UNKNOWN, L_SYNCHRO,
      L_PROGRESS, L_TIMEJUMP, L_SEC, L_MIN, L_HOUR, L_DAY, L_DAYS, L_END};

#ifdef DEFVAR
char	* Label[L_END] = {
	"Point", "GMT time", "Solar time", "Legal time", 
        "Day length", "Sunrise", "Sunset", 
        "Click on a city",
        "Click on a location",
        "Click consecutively on two locations",
        "Double click or strike * for ° ' \"",
	"Option",
        "Activating selected option...",
        "Option incorrect or not available at runtime !!",
	"Key",
        "Key/Mouse controls",
	"Escape",
        "Escape menu",
	"Unknown key binding !!",
        "Synchro",
	"Progress value =",
        "Global time shift =",
	"seconds",
	"minute",
	"hour",
	"day",
        "days"
};

char    MenuKey[2*N_MENU] = 
"H,F,Z,O;C,S,D,E,L;A,B,G,J;N,Y,U;M,P,T;W,K,I,R;>,<,!;X,Q;";

char    ZoomKey[2*N_ZOOM] =
"*,#,/;&,+,-,1,.;>,<,!,W,K;";

char    OptionKey[2*N_OPTION] = 
"@,%;=;!,W,K;";

char    CommandKey[N_HELP] = 
"HFZOCSDELABGJNYUMPTWKIR><!XQ*#/&+-1.\"@%=";

char  * Help[N_HELP] = {

/* Menu window help */
"Show help and commands (H or click on strip)",
"Load Earth map file (F or mouse button 2)",
"Zoom (Z or mouse button 3)",
"Option command window",
"Use coordinate mode",
"Use solar time mode",
"Use distance mode",
"Use hour extension mode",
"Use legal time mode",
"Modify time forwArd",
"Modify time Backward",
"Adjust proGress value",
"Reset global time shift to Zero",
"Draw/Erase Night area",
"Draw/Erase Sun",
"Draw/Erase cities",
"Draw/Erase meridians",
"Draw/Erase parallels",
"Draw/Erase Tropics/Equator/Arctic circles",
"Open new map window (W or Mousebutton 3)",
"Close window",
"Iconify window",
"Refresh map window",
"Adjust window width to screen size",
"Back to previous window size",
"Switch clock and map windows",
"Activate command (-command option)",
"Quit program",

/* Zoom window help */
"Activate new zoom settings",
"Return to previous zoom settings",
"Set aspect by resizing main window",
"Cycle through zoom modes 0,1,2",
"Zoom in by factor 1.2",
"Zoom out by factor 1/1.2 = 0.833",
"Return to zoom factor = 1 (full map)",
"Center zoom area on selected city or location",
"Synchronize zoom operation",

/* Option window help */
"Activate the selected option",
"Erase the option command line",
"Synchronize windows or not"
};

char	*ShortHelp = 
"Sunclock has a number of internal procedures which can be accessed\n"
"through mouse clicks or key controls:";

char	*Configurability = 
"Starting from **, options are runtime configurable.";

char  	Day_name[7][10] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char    Month_name[12][10] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec" 
};

#else

extern char Day_name[7][10];
extern char Month_name[12][10];
extern char MenuKey[N_MENU];
extern char ZoomKey[N_ZOOM];
extern char OptionKey[N_OPTION];
extern char CommandKey[N_HELP];

#endif
