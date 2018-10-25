/* 
 *	Remember to compile try:
 *			1) gcc hi.c -o hi -lX11
 *			2) gcc hi.c -I /usr/include/X11 -L /usr/X11/lib -lX11
 *			3) gcc hi.c -I /where/ever -L /who/knows/where -l X11
 *  g++ sdfviewer.o sdfviewer.c -o sdfviewer -lX11 -std=c++11 && ./sdfviewer
 *
 *
 *			Brian Hammond 2/9/96.    Feel free to do with this as you will!
 *      Thoth Gunter 2018.       Thanks Brian! I think I will :)
 *      !!! Original X11 window creation source code by Brian Hammond.
 *      Heavily modified by Thoth Gunter many moons later !!!
*/


//TODO
//+ strings
//+ integrate log scale
//+ button struct
//+ additional statistics

/* include the X library headers */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

/* C/C++ libs */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <sstream>


/* linux libs */
#include <unistd.h>
#include <dirent.h>

/* SDF libs */
#include "SDF.cc"


//Icon info
extern const char _binary_sdfviewer_bmp_start;
extern const char _binary_sdfviewer_bmp_end;
extern const int  _binary_sdfviewer_bmp_size;


struct TGHistogram{
    std::vector<int> contents;
    std::vector<float> edges;
};

struct TGAxis{
    std::vector<float> xticks; 
    std::vector<float> yticks; 
};
struct TGAxisLabels{
    std::vector<std::string> xlabels; 
    std::vector<std::string> ylabels; 
};
enum TGScale{
    LINEAR,
    LOG
};
struct TGPlot{
    bool initialize = false;
    TGAxis axis;
    TGScale scale;
    TGAxisLabels labels;
};


struct  TGBmpheader{
    char signature[2];
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
};
int load_bmpheader(const char* buffer, TGBmpheader* header);


//GLOBAL
/* here are our X variables */
Display *dis;
int screen;
Window win;
GC gc;
static Atom wm_delete_window;



int window_width  = 1000;
int window_height = 600 ;

int menu_x = 70;
int menu_y = 0;



/* here are our X routines declared! */
void init_x();
void close_x();
void redraw();

/* PERSONAL RENDER API */
void TGDrawRectangle(int x, int y, int w, int h);
void TGFillRectangle(int x, int y, int w, int h);
void TGDrawLine(int x1, int x2, int y1 ,int y2);
void TGDrawString(int x, int y, std::string str);
bool TGInRectangle(int x, int y, int x_rect, int y_rect, int w_rect, int h_rect);
int TGDrawHistogram( std::vector<int> contents, std::vector<float> edges, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height, bool fill, TGPlot* plot = NULL);
void TGDrawTicks(TGPlot* plot, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height);
void TGDrawTickLabels(TGPlot* plot, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height);
TGHistogram TGConstructHistogram_float( std::vector<float> data, float min=-1.0, float max=-1.0 );
TGHistogram TGConstructHistogram_int( std::vector<int> data, int min=-1, int max=-1 );

//NOTE
//copied from the solution on 
//https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 2)
{
      std::ostringstream out;
      out.precision(n);
      out << a_value;
      return out.str();
}
/* */


int main (int n_command_line_args, char** argv) {
    //NOTE
    //Get command line arguments
    //parse for file name
    std::string file_name = "test_compression_0.sdf";
    if (n_command_line_args > 1 ){
        if ( access(argv[1], F_OK) != 0 ){
            printf("FILE  %s  does not exist!\n", argv[1]);
            return -1;
        }
        else{
            printf("LOADING FILE  %s \n", argv[1]);
            file_name = argv[1];
        }
    }


    //TODO
    //BMP test
    TGBmpheader bmpheader;
    load_bmpheader( &_binary_sdfviewer_bmp_start, &bmpheader);



    //NOTE
    //Get sdf information and data
		std::fstream f;
		f.open(file_name.c_str(), std::fstream::in | std::fstream::binary);
		if(!f.good()){
				printf("File is note good!");
				return -1;
		}
    headerSDF header = read_sdfheader_from_disk(&f);
    //TODO 
    //make more robust. We should not shut down just because the file was bad.
    if (_sdfERRNO != 0) {
        printf("Improper file format!");
        return -1;
    } 


    std::vector<std::string> menu_arr;
    int menu_index = 0;
    int menu_length = 20;
    std::vector<bool> active_features;
    bool share = false; 
    int init_shared = -1; 
    bool plot_changed = false; 

    //NOTE
    //base_plot_index is the index of the bottom most shaded histogram when histograms
    //share a plot.
    int base_plot_index = -1;
    
    //NOTE
    //Construct feature menu
    {
        for(int i = 0; i < header.type_tags.size(); i++){
              menu_arr.push_back(header.type_tags[i].name);
              active_features.push_back(false);
        }
    }
    if (menu_length > menu_arr.size()) menu_length = menu_arr.size();


		XEvent event;		
		KeySym key;		  
		char text[255];	

		int plot_bound_x      = 200;
		int plot_bound_y      = 100;
		int plot_bound_width  = window_width - plot_bound_x - 50 ;
		int plot_bound_height = window_height - plot_bound_y - 50;

		init_x();

		/* look for events forever... */
		while(1) {		
				XNextEvent(dis, &event);
			
        if (event.type == ClientMessage){
            if((Atom)event.xclient.data.l[0] == wm_delete_window){
                close_x();
            }
        }
				if (event.type == KeyPress && XLookupString(&event.xkey, text, 255, &key, 0) == 1) {
				/* use the XLookupString routine to convert the invent
		 *		   KeyPress data into regular text.  Weird but necessary...
		 *	*/
						if (text[0]=='q') {
								close_x();
						}
				}
				if (event.type==ButtonPress) {
						int x = event.xbutton.x;
						int	y = event.xbutton.y;

						//Note
						//Setting shared histogram flag
						{
								if( TGInRectangle(x, window_height - y, 20, plot_bound_y + plot_bound_height - 30, 70, 20) ){ 
										if( share ) share = false;
										else{
                        share = true;
                        //TODO 
                        //loop through active features to determin 
                        init_shared = 0;
                    }
								}
						}
						//Note
						//Setting active feature
						{
								for(int i = 0; i < menu_length; i++){
										if (TGInRectangle(x, window_height - y, 10 , 500 - 25 * i - 5, 100, 20 )){
                        int ith = i + menu_index;
                        if( ith > menu_arr.size() ) ith = menu_arr.size() - ith;
            
                        if( base_plot_index == -1 ) base_plot_index = ith; 
												if (active_features[ith] ) active_features[ith] = false;
												else{
                            active_features[ith] = true;
                        }
                        {
                            bool no_active_features = true;
                            for(int j = 0; j < menu_arr.size(); j++){
                                if (share == false){
                                    if( ith != j ) active_features[j] = false;
                                }
                                if ( active_features[j] ) no_active_features = false;
                            }
                            if( no_active_features ) base_plot_index = -1;
												}
                        plot_changed = true;
												break;
										}
								}
						}
            {//Arrows that scroll through features
                if (TGInRectangle(x, window_height - y, 50 + menu_x, 450, 10, 25)) {
                    //TODO
                    //Give user some feed back that the interaction with the
                    //button has been registered.
                    if( menu_length < active_features.size()){
                        menu_index++;
                        if (menu_index >= active_features.size()) menu_index = 0;
                    }
                }
                if (TGInRectangle(x, window_height - y, 70 + menu_x, 450, 10, 25)){
                    //TODO
                    //Give user some feed back that the interaction with the
                    //button has been registered.
                    if( menu_length < active_features.size()){
                        menu_index--;
                        if (menu_index < 0) menu_index = active_features.size()-1;
                    }
                }
            }
				}

				if ((event.type == Expose && event.xexpose.count==0) || event.type==ButtonPress ) {
						if ( plot_changed ) redraw();
				}

				//NOTE
				//debug toggle share button
				if (share){
						XSetForeground(dis, gc, 0xffaa5555);
						TGFillRectangle(20, plot_bound_y + plot_bound_height - 5 - 30, 70, 20);
				}
				else{
						XSetForeground(dis, gc, 0xff55aa55);
						TGFillRectangle(20, plot_bound_y + plot_bound_height - 5 - 30, 70, 20);
				}

				//NOTE
				// Setting up background highlighing for selected features.
				XSetForeground(dis, gc, 0xffaaaaaa);
				for(int i = 0; i < menu_length; i++){
            int ith = i + menu_index;
            if( ith > menu_arr.size() ) ith = menu_arr.size() - ith;
						if ( active_features[ith] ) {
								TGFillRectangle(10, 500 - 25 * i - 5, 100, 20);
						}	
				}
				XSetForeground(dis, gc, 0xffffffff);
				TGDrawString(10, window_height - 50, file_name.c_str());

				{//Draw feature menu
						int i = 0;
						while(i < menu_length){
                int ith = i + menu_index;
                if( ith > menu_arr.size() ) ith = menu_arr.size() - ith;
								TGDrawString(10, 500 - 25 * i, menu_arr[ith] );
								i++;
						}
				}

        {// Movin through menu list
            //TODO 
            //Draw very dark grey bounding
            //box around arrows and hook up 
            //event detection
            XSetForeground(dis, gc, 0xff000000);
            TGFillRectangle(50 + menu_x, 450, 10, 25);
            TGFillRectangle(70 + menu_x, 450, 10, 25);
            
				    XSetForeground(dis, gc, 0xffffffff);
            TGDrawLine(50 + menu_x, 60 + menu_x, 450, 450);
            TGDrawLine(60 + menu_x, 55 + menu_x, 450, 475); 
            TGDrawLine(55 + menu_x, 50 + menu_x, 475, 450); 

            TGDrawLine(70 + menu_x, 80 + menu_x, 475, 475);
            TGDrawLine(80 + menu_x, 75 + menu_x, 475, 450); 
            TGDrawLine(75 + menu_x, 70 + menu_x, 450, 475); 
        }

        {//File stats
				    XSetForeground(dis, gc, 0xffffffff);
            TGDrawString(plot_bound_x + 5, plot_bound_y + plot_bound_height + 10, "number of entries: " + to_string_with_precision(header.number_of_events)  );
        }


        //TODO
        //Global maybe???
        unsigned int colors_TEMP[] = {0xffaa00dd, 0xff47b0e0, 0xff5f48e3};

        TGPlot plot;
        plot.initialize = false;
        
				//SOME OF THESE ARE FAKE HISTOGRAMS
        if(plot_changed){
            for(unsigned int _i_feature = 0; _i_feature < active_features.size() ; _i_feature++){
                //NOTE
                if ( base_plot_index == -1 ) break;
                int i_feature = _i_feature + base_plot_index;
                if ( i_feature >= active_features.size() ) {
                    i_feature = i_feature - int(active_features.size());
                }

                if (active_features[i_feature]){
                    XSetForeground(dis, gc, colors_TEMP[i_feature]);
                    tagHeaderSDF tag;
                    for(int _i_tags = 0; _i_tags < header.type_tags.size(); _i_tags++){
                        if( header.type_tags[_i_tags].name == menu_arr[i_feature]) tag = header.type_tags[_i_tags];
                    }
                    std::vector<uint8_t> _data = get_buffer_from_disk( &f, &tag, &header);
                    TGHistogram histogram;
                    if ( tag.type_info == FLOAT ) {
                        std::vector<float> data((float*)&_data[0],(float*)&_data[0] + _data.size() / 32);
                        if( share && plot.initialize == true ) histogram = TGConstructHistogram_float(data, plot.axis.xticks[0], plot.axis.xticks[ plot.axis.xticks.size() - 1 ]);
                        else histogram = TGConstructHistogram_float(data);
                    }
                    else if ( tag.type_info == INT ) {
                        std::vector<int> data((int*)&_data[0],(int*)&_data[0] + _data.size() / 32);
                        if( share && plot.initialize == true ) histogram = TGConstructHistogram_int(data, plot.axis.xticks[0], plot.axis.xticks[ plot.axis.xticks.size() - 1 ]);
                        else histogram = TGConstructHistogram_int(data);
                    }
                    else{
                        std::cout << "NOT COMPLETED" << std::endl;
                        break;
                    }

                    if ( share ) {
                        if ( i_feature == base_plot_index ) {
                            TGDrawHistogram( histogram.contents, histogram.edges ,plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height, true, &plot);
                        }
                        else{
                            TGDrawHistogram( histogram.contents, histogram.edges ,plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height, false, &plot);
                        }
                    }
                    else{ 
                        TGDrawHistogram( histogram.contents, histogram.edges ,plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height, true, &plot);
                    }
                }
                //TODO
                //Handle strings
                if (active_features[i_feature] && i_feature == 2){
                    XSetForeground(dis, gc, 0xff5f48e3);
                    std::vector<int> contents = {100, 250, 300, 250, 200};
                    std::vector<float> edges = {50, 50*2, 50*3, 50*4, 50*5, 50*6};

                    TGDrawHistogram( contents, edges ,plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height, true);
                }
            }
				}	
        plot_changed = false;
				XSetForeground(dis, gc, 0xffdddddd);
				TGDrawRectangle(plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height);
				TGDrawString(plot_bound_x + plot_bound_width/2, plot_bound_y - 50, "logy");
				TGDrawString(42 - 20, plot_bound_y + plot_bound_height - 30, "share");
				TGDrawString(42 + 20, plot_bound_y + plot_bound_height - 30, "full");
		}
}

void TGDrawString(int x, int y, std::string str){
		XDrawString(dis, win, gc, x, window_height - y, str.c_str(), str.size());
}

void TGFillRectangle(int x, int y, int w, int h){
		XFillRectangle(dis, win, gc, x, window_height - y - h, w, h);
}
void TGDrawRectangle(int x, int y, int w, int h){
		XDrawRectangle(dis, win, gc, x, window_height - y - h, w, h);
}
void TGDrawLine(int x1, int x2, int y1 ,int y2){
		XDrawLine(dis, win, gc, x1, window_height - y1, x2, window_height - y2);
}
bool TGInRectangle(int x, int y, int x_rect, int y_rect, int w_rect, int h_rect){
		bool rt = true; 
		if (x > x_rect && x < x_rect + w_rect)  rt &= true;
		else rt &= false;
		if (y > y_rect && y < y_rect + h_rect) rt &= true;
		else rt &= false;
		return rt;
}

//NOTE
//Maybe this should return a TGPlot
int TGDrawHistogram( std::vector<int> contents, std::vector<float> edges, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height, bool fill, TGPlot* plot){
		if( contents.size() + 1 != edges.size()) return -1;
    int max_content = 0;
    float max_edge = edges[edges.size() - 1];
    //NOTE
    //I cant move this inside an if statement without core dump and I don't know why! 
    for(int i= 0; i < contents.size(); i++){
          if(contents[i] > max_content ) max_content = contents[i];
    }
    float scale = plot_bound_height / (1.20 * max_content);


    TGPlot _plot;
    if (plot == NULL){
        plot = &_plot;
        plot->initialize = false;
    }
    if (plot->initialize == false){
        for(int i_edge = 0; i_edge < edges.size(); i_edge++){
            plot->axis.xticks.push_back(edges[i_edge]);
            plot->labels.xlabels.push_back(to_string_with_precision(edges[i_edge]));
        }
        float prev = 0;
        for(int y = 0; y < max_content * 1.20; y++){
            if ( max_content / (y - prev)  <= 20 || y == 0){
                prev = y;
                plot->axis.yticks.push_back(y);
                plot->labels.ylabels.push_back(to_string_with_precision(y));
            }
        }
        plot->initialize = true;
    }
    else{
        max_content = plot->axis.yticks[plot->axis.yticks.size() - 1] / 1.20;
        scale = plot_bound_height / (1.20 * max_content);
    }

    std::vector<float> _edges;
    for(int i= 0; i < edges.size(); i++){

        _edges.push_back(round( plot_bound_width * (edges[i] - edges[0]) / (max_edge - edges[0])));
    }
    

    for(int i= 0; i < contents.size(); i++){
        //TODO
        //cap height to the plot bound height
        int height  = int(scale * contents[i]) >= plot_bound_height ? plot_bound_height :  int(scale * contents[i]);
        if (fill) TGFillRectangle(int(plot_bound_x + _edges[i]), plot_bound_y, int(_edges[i+1] - _edges[i]), height);
        else TGDrawRectangle(int(plot_bound_x + _edges[i]), plot_bound_y, int(_edges[i+1] - _edges[i]), height);
    }
    TGDrawTicks(plot, plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height);
    //TODO
    TGDrawTickLabels(plot, plot_bound_x, plot_bound_y, plot_bound_width, plot_bound_height);
		return 0;
}

void TGDrawTicks(TGPlot* plot, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height){
    float x_min = plot->axis.xticks[0];
    float x_max = plot->axis.xticks[plot->axis.xticks.size() - 1];

    float y_min = plot->axis.yticks[0];
    float y_max = plot->axis.yticks[plot->axis.yticks.size() - 1];

		XSetForeground(dis, gc, 0xffffffff);
    for(int i_x = 0; i_x < plot->axis.xticks.size(); i_x++){
        float _x = plot->axis.xticks[i_x];
        int x_axis_tick = int ( round( plot_bound_width * (_x - x_min) / ( x_max - x_min ) ) ) + plot_bound_x;
        TGDrawLine(  x_axis_tick, x_axis_tick, plot_bound_y - 5, plot_bound_y + 5 );
    }
    for(int i_y = 0; i_y < plot->axis.yticks.size(); i_y++){
        float _y = plot->axis.yticks[i_y];
        int y_axis_tick = int ( round( plot_bound_height * (_y - y_min) / ( y_max - y_min ) ) ) + plot_bound_y;
        TGDrawLine( plot_bound_x - 5, plot_bound_x + 5, y_axis_tick, y_axis_tick );
    }
}
void TGDrawTickLabels(TGPlot* plot, int plot_bound_x, int plot_bound_y, int plot_bound_width, int plot_bound_height){
    float x_min = plot->axis.xticks[0];
    float x_max = plot->axis.xticks[plot->axis.xticks.size() - 1];

    float y_min = plot->axis.yticks[0];
    float y_max = plot->axis.yticks[plot->axis.yticks.size() - 1];

		XSetForeground(dis, gc, 0xffffffff);
    for(int i_x = 0; i_x < plot->axis.xticks.size(); i_x++){
        float _x = plot->axis.xticks[i_x];
        int x_axis_tick = int ( round( plot_bound_width * (_x - x_min) / ( x_max - x_min ) ) ) + plot_bound_x;
        TGDrawString(  x_axis_tick - 5, plot_bound_y - 15, plot->labels.xlabels[i_x] );
    }
    for(int i_y = 0; i_y < plot->axis.yticks.size(); i_y++){
        float _y = plot->axis.yticks[i_y];
        int y_axis_tick = int ( round( plot_bound_height * (_y - y_min) / ( y_max - y_min ) ) ) + plot_bound_y;
        TGDrawString( plot_bound_x - 25,  y_axis_tick, plot->labels.ylabels[i_y] );
    }
    
}

TGHistogram TGConstructHistogram_float( std::vector<float> data, float min, float max ){
    float mean = 0;
    bool min_max_set = false;
    if( min == max){
        min = 99999.0;
        max = -99999.0;
    }
    else{
        min_max_set = true;
    }
    
    for (int i= 0; i < data.size(); i++){
        mean += data[i];
        if( min_max_set == false ) {
            if ( min > data[i] ) {
                min = data[i];
            }
            if ( max < data[i] ) {
                max = data[i];
            }
        }
    }
    mean = mean / data.size();

    //NOTE use mean or some other statistic to pick better bin
    float bin_size = 0;//fabs( max - min ) / 20;
    if (fabs(max - min) >= 20 ) {bin_size = fabs( max - min ) / 20;}
    else bin_size = fabs( mean - min );
    TGHistogram rt;
    {//Initialize histogram
        rt.edges.push_back(min);
        for(int i = 0; i < 20; i++){
            rt.contents.push_back(0);
            rt.edges.push_back(min + bin_size * (i+1));
        }
    }
    for(int i = 0; i < data.size(); i++){
        for(int j= 0; j < 20; j++){
            if( data[i] > min + bin_size * j && data[i] < min + bin_size * (j + 1) ) {
                rt.contents[j] += 1;
            }
        }
    }
    return rt;
}

TGHistogram TGConstructHistogram_int( std::vector<int> data, int min, int max ){
    int mean = 0;
    bool min_max_set = false;
    if ( min == max ){
        min = 99999;
        max = -99999;
    }
    else{
        min_max_set = true;
    }
    
    for (int i= 0; i < data.size(); i++){
        mean += data[i];
        if( min_max_set == false ) {
            if ( min > data[i] ) {
                min = data[i];
            }
            if ( max < data[i] ) {
                max = data[i];
            }
        }
    }
    mean = mean / data.size();

    //NOTE use mean or some statistic to pick better bin
    int bin_size = 0;
    if (abs(max - min) >= 20 ) {bin_size = abs( max - min ) / 20;}
    else bin_size = abs( mean - min );
    TGHistogram rt;
    {//Initialize histogram
        rt.edges.push_back(min);
        for(int i = 0; i < 20; i++){
            rt.contents.push_back(0);
            rt.edges.push_back(min + bin_size * (i+1));
        }
    }
    for(int i = 0; i < data.size(); i++){
        for(int j= 0; j < 20; j++){
            if( data[i] > min + bin_size * j && data[i] < min + bin_size * (j + 1) ) {
                rt.contents[j] += 1;
            }
        }
    }

    return rt;
}


void init_x() {
/* get the colors black and white (see section for details) */        
		unsigned long black,white;

		dis=XOpenDisplay((char *)0);
		screen=DefaultScreen(dis);
		black=BlackPixel(dis, screen),
		white=WhitePixel(dis, screen);
		win=XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, window_width, window_height, 5, black, black);

		XSetStandardProperties(dis,win,"SDF Viewer","SDF Viewer v01",None,NULL,0,NULL);
		XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);
		gc=XCreateGC(dis, win, 0,0);        

		XSetBackground(dis, gc, 0);
		XSetForeground(dis, gc, white);
		XClearWindow(dis, win);
		XMapRaised(dis, win);

    wm_delete_window = XInternAtom(dis, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dis, win, &wm_delete_window, 1);
};

void close_x() {
		XFreeGC(dis, gc);
		XDestroyWindow(dis, win);
		XCloseDisplay(dis);	
		exit(1);				
};

void redraw() {
		XClearWindow(dis, win);
};

//NOTE
//The following was derived in part from
//https://stackoverflow.com/questions/10699927/xlib-argb-window-icon/10714086#10714086
struct  TGBmpDIB{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t plane;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t horizontal_res;
    uint32_t vertical_res;
    uint32_t colors_used;
    uint32_t important_colors;
};
struct TGColor{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved;
};
struct  TGColorTable{
    std::vector<TGColor> data;    
};
struct  TGPixelData{
    std::vector<unsigned int8_t> data;    
};
struct TGBmp{
    TGBmpheader header;
    TGBmpDIB dib;
    TGColorTable color_table;
    TGPixelData pixel_data;
};
int load_bmpheader(const char* buffer, TGBmpheader* header){
    if (buffer[0] != 'B' || buffer[1] != 'M') return -1;
    header->signature[0] = buffer[0];
    header->signature[1] = buffer[1];
    //TODO
    //This is not right
    //The index does not make any sense 
    header->file_size   = ((uint32_t*) &buffer[2])[0];
    header->reserved    = ((uint32_t*) &buffer[2])[1];
    header->data_offset = ((uint32_t*) &buffer[2])[2];
    return 0;
};
int load_bmpdib(const char* buffer, TGBmpDIB* dib){
    return 0;
};

/*
void bmp_from_buffer( char* buffer, TGBmp bmp){
};
void set_icon()
{
    unsigned long buffer[100]; 
    Atom net_wm_icon = XInternAtom(dis, "_NET_WM_ICON", False);
    Atom cardinal = XInternAtom(d, "CARDINAL", False);
    w = XCreateWindow(d, RootWindow(d, s), 0, 0, 200, 200, 0, CopyFromParent, InputOutput, CopyFromParent, 0, 0);
    int length = 2 + 16 * 16 + 2 + 32 * 32;
    XChangeProperty(d, w, net_wm_icon, cardinal, 32, PropModeReplace, (const unsigned char*) buffer, length);
};

*/