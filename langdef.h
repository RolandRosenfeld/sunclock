#define STDFORMATS "%H:%M%_%a%_%d%_%b%_%y|%H:%M:%S%_%Z|%a%_%j/%t%_%U/52"

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
	"New map/window",
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

#define N_ZOOM 7
#define ZOOMMENU "*#&+-1.><!WK"

#define N_OPTIONS 28
char    Option[2*N_OPTIONS] = 
"H,F,Z;C,S,D,E,L;A,B,G,J;N,O,U;M,P,T;W,K,I,R;>,<,=,!;X,Q;";

char	Help[N_OPTIONS+N_ZOOM+1][80] = {
"Show help and options (H or click on strip)",
"Load Earth map file (F or mouse button 2)",
"Zoom (Z or mouse button 3)",
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
"Synchronize windows or not",
"Switch clock and map windows",
"Activate command (-command option)",
"Quit program",
"Escape menu",
"Activate new zoom settings",
"Return to previous zoom settings",
"Cycle through zoom modes 0,1,2",
"Zoom in by factor 1.2",
"Zoom out by factor 1/1.2 = 0.833",
"Return to zoom factor = 1 (full map)",
"Center zoom area on selected city or location"
};

char	ShortHelp[512] = 
"Sunclock has a number of internal procedures which can be accessed\n"
"through mouse clicks or key controls:";







