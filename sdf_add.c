/* C/C++ libs */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>


/* linux libs */
#include <unistd.h>
#include <dirent.h>


#include "SDF.cc"

//  g++ sdf_add.c -o sdf_add -std=c++11 && ./sdf_add



//TODO
//Concate many sdf files 

int main(int n_args, char ** args){
    //NOTE
    //Get command line arguments
    //parse for file name
    std::string file_name_out = "test_out.sdf";
    std::vector<std::string> list_file_in;
    
    if (n_args > 2 ){
        for( int i = 1; i < n_args; i++){ 
            if( i == 1 ) file_name_out = args[i];
            else{
                if ( access(args[i], F_OK) != 0 ){
                    printf("FILE \" %s \" does not exist!\n", args[i]);
                    return -1;
                }
                else{
                    printf("LOADING FILE \" %s \" \n", args[i]);
                    list_file_in.push_back( args[i] );
                }
            }
        }
    }
    else{
        printf("Incorrect usage of application\n");
        printf("Example\n sdf_add concate_file file1 file2 file3");
        return -1;
    }
   
    //NOTE 
    //If we ever want to use wildcards we will need this
    /*
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if(d){
        while ( (dir = readdir(d)) != NULL){
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    */



    //NOTE
    //Initialize SDF file
    std::fstream ZERO_f;
    ZERO_f.open(list_file_in[0].c_str(), std::fstream::in | std::fstream::binary);

    if(!ZERO_f.good()){
        printf("File is note good!");
        return -1;
    }
    headerSDF ZERO_header = read_sdfheader_from_disk(&ZERO_f);


    SerialDataFormat out_sdf;
    
    //NOTE
    //I'm not sure this needs to be done
    //
    std::map<std::string, float>        float_map;
    std::map<std::string, int>          int_map;
    std::map<std::string, std::string>  string_map;
  
    out_sdf.vars_float  = &float_map;
    out_sdf.vars_int    = &int_map;
    out_sdf.vars_str    = &string_map;

    for( int ith_feature = 0; ith_feature < ZERO_header.number_of_features; ith_feature++ ){
        if(ZERO_header.type_tags[ith_feature].type_info == FLOAT)     float_map[ ZERO_header.type_tags[ith_feature].name ]  = 0.0;
        else if(ZERO_header.type_tags[ith_feature].type_info == INT)  int_map[ ZERO_header.type_tags[ith_feature].name ]    = 0;
        else if(ZERO_header.type_tags[ith_feature].type_info == STR)  string_map[ ZERO_header.type_tags[ith_feature].name ]   = "";
        else{
            //TODO
            //We prob should not kill the program if we don't know the type currently.
            //We really only want to skip said feature and print a warning.
            printf("THIS TYPE IS CURRENTLY NOT HANDLED!!!!");
            return -1;
        }
    }
    stage_variables( &out_sdf );
    ZERO_f.close();





    for(int ith_file = 0; ith_file < list_file_in.size(); ith_file++){
        //NOTE
        //Get sdf information and data
        std::fstream f;
        f.open(list_file_in[ith_file].c_str(), std::fstream::in | std::fstream::binary);

        if(!f.good()){
            printf("File is not good!");
            return -1;
        }

        headerSDF header = read_sdfheader_from_disk(&f);

        //TODO
        //Check to make sure that the feature list matches
        //Throw and error or warning if they do not
        int features_shared = 0;
        for(int ith_tag = 0; ith_tag < out_sdf.header.type_tags.size(); ith_tag++){
            bool files_share_features = false; 
            for(int ith_opentag = 0; ith_opentag < header.type_tags.size(); ith_opentag++){
                if( out_sdf.header.type_tags[ith_tag].type_info == header.type_tags[ith_tag].type_info &&  out_sdf.header.type_tags[ith_tag].name == header.type_tags[ith_tag].name  ) {
                    files_share_features |= true;
                }
            }
            if (files_share_features) features_shared++;
        } 
       
        if ( features_shared != out_sdf.header.number_of_features ) {
            return -1;
        }

        //TODO 
        //make more robust. We should not shut down just because the file was bad.
        if (_sdfERRNO != 0) {
            printf("Improper file format!");
            return -1;
        }
        //NOTE
        //Loop over features 
        //add data to out files
        //update out header with the correct size data stream

        out_sdf.header.number_of_events += header.number_of_events;
        for(int ith_tag = 0; ith_tag < out_sdf.header.type_tags.size(); ith_tag++){
            std::vector<uint8_t> _data = get_buffer_from_disk( &f, &out_sdf.header.type_tags[ith_tag], &header);
            out_sdf.buffer[ith_tag].buffer.insert( out_sdf.buffer[ith_tag].buffer.end(), _data.begin(), _data.end() );
        }
        f.close();
    }
    write_sdf_to_disk( file_name_out.c_str() , &out_sdf );
    return 0;
}


//TODO
int test_function(){
  //TODO
  //Generate two files
  //Concate the two files
  //Check the two files to be sure 
}









