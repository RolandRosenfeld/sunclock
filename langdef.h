char  	Day_name[7][10] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char    Month_name[12][10] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec" 
};

enum {L_POINT=0, L_GMTTIME, L_SOLARTIME, L_LEGALTIME, L_DAYLENGTH,
      L_SUNRISE, L_SUNSET, L_CLICKCITY, L_CLICKLOC, L_CLICK2LOC, 
      L_KEY, L_CONTROLS, L_CANCEL, 
      L_PROGRESS, L_GLOBALSHIFT, L_HOURS, L_DAYS, L_ENTER, L_END};

char	Label[L_END][60] = {
	"Point", "GMT time", "Solar time", "Legal time", "Day length",
        "Sunrise", "Sunset", 
        "Click on a city",
        "Click on a location",
        "Click consecutively on two locations",
	"Key",
        "Key/Mouse controls",
	"Escape",
	"Progress value =",
        "Global time shift =",
	"hours",
	"days",
        "Type A,B,G,Z or Enter"
};

#define N_OPTIONS 18
char    Option[2*N_OPTIONS] = 
"C,D,L,S;A,B,G,Z;M,P,T,U;H,I,R,W,X,Q;";
char	ListOptions[4*N_OPTIONS+2];

char	Help[N_OPTIONS][80] = {
"Use coordinate mode",
"Use distance mode",
"Use legal time mode",
"Use solar time mode",
"Modify time forwArd",
"Modify time Backward",
"Adjust time proGress value",
"Reset global time shift to Zero",
"Draw/Erase meridians",
"Draw/Erase parallels",
"Draw/Erase Tropics/Equator/Arctic circles",
"Unmark/Draw cities",
"Show help and options",
"Iconify window",
"Refresh map window",
"Switch clock and map windows",
"Activate command (-cmd option)",
"Quit program"
};

char	ShortHelp[512] = "\
Sunclock has a number of internal procedures which can be accessed
through mouse clicks or key controls:";
