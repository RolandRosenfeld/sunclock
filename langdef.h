char  	Day_name[7][10] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char    Month_name[12][10] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec" 
};

enum {L_POINT=0, L_GMTTIME, L_SOLARTIME, L_LEGALTIME,
      L_DAYLENGTH, L_SUNRISE, L_SUNSET, 
      L_CLICKCITY, L_CLICKLOC, L_CLICK2LOC, L_DEGREE, L_VECTMAP,
      L_KEY, L_CONTROLS, L_ESCAPE, 
      L_PROGRESS, L_TIMEJUMP, L_SEC, L_MIN, L_HOUR, L_DAY, L_DAYS, L_END};

char	Label[L_END][60] = {
	"Point", "GMT time", "Solar time", "Legal time", 
        "Day length", "Sunrise", "Sunset", 
        "Click on a city",
        "Click on a location",
        "Click consecutively on two locations",
        "Double click or strike * for ° ' \"",
	"Internal vector map",
	"Key",
        "Key/Mouse controls",
	"Escape",
	"Progress value =",
        "Global time shift =",
	"seconds",
	"minute",
	"hour",
	"day",
        "days"
};

#define N_OPTIONS 27
char    Option[2*N_OPTIONS] = 
"F,+,-,<;C,S,D,E,L;A,B,G,Z;N,O,U;M,P,T; ,W,K,I,R;H,X,Q;";
char	ListOptions[4*N_OPTIONS+2];

char	Help[N_OPTIONS+1][80] = {
"Load Earth map file",
"Zoom +",
"Zoom -",
"Back to previous window size",
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
"Switch clock and map windows",
"Open new map window (W or Mousebutton 3)",
"Close window",
"Iconify window",
"Refresh map window",
"Show help and options",
"Activate command (-cmd option)",
"Quit program",
"Escape menu"
};

char	ShortHelp[512] = 
"Sunclock has a number of internal procedures which can be accessed\n"
"through mouse clicks or key controls:";







