#include "miniz.h"
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <stdlib.h>     /* exit, EXIT_FAILURE */


//TODO
// Make thing more C-like to ease use with other languages
// + 16bit float


//NOTE
//dll compile
//g++ -c SDF.cc -std=c++11 -fPIC  &&  g++ -shared -o SDF.dll SDF.o



#ifdef __cplusplus
extern "C" {
#endif


int _sdfERRNO = 0;

enum TypeTag{
	FLOAT = 0,
	INT,
	STR 
};


struct stringSDF{
		char characters[256];
    stringSDF(){
        for( unsigned int i = 0; i < sizeof(characters); i++){
            characters[i] = '\0';
        }
    }
};


struct tagHeaderSDF{
		std::string name;
		TypeTag type_info;
		uint32_t buffer_size;
};

struct headerSDF{
		char ID[3] = {'T', 'D', 'F'};
		uint8_t compression_flag= 1;
		uint32_t start_buffer = 3 + 1 + 4 + 4 + 4 + 256;
		//NOTE:
		//		CHECK FOR Endianess
		//    union {
		//       uint32_t i;
		//       char c[4];
		//    } bint = {0x01020304};
		//
		//    bint.c[0] == 1; 
		//NOTE:
		//Currently we store up to 4billion events and no more
		uint32_t number_of_events = 0;
		uint32_t number_of_features = 0; //32bytes is a little large don't yah think
		std::vector<tagHeaderSDF> type_tags;
    stringSDF notes;
};	

//Note:
//Generally for any given feature I want to store information for all events in one continuous structure.
//Not the opposite, ie roots model. 
struct memblockSDF{
		//NOTE:
		//Compress the buffer with this
		//https://github.com/richgel999/miniz
		std::vector<uint8_t> buffer;
};

struct SerialDataFormat{
	
		headerSDF header;
		std::vector<memblockSDF> buffer;

		//NOTE:
		//We do not save this out!!!
		//This is only for updating data!
		std::map<std::string, float>* vars_float;
		std::map<std::string, int>* vars_int;
		std::map<std::string, std::string>* vars_str;
};

int stage_variables( SerialDataFormat* sdf );
void update_buffers(SerialDataFormat* sdf);
void write_sdf_to_disk(std::string file_name, SerialDataFormat* sdf );
headerSDF read_sdfheader_from_disk(std::fstream* f);
int get_tag_index(headerSDF* header, TypeTag type , std::string name);
std::vector<uint8_t> get_buffer_from_disk( std::fstream* f, tagHeaderSDF* tag, headerSDF* header);
SerialDataFormat read_sdf_from_disk(std::string file_name);



#ifdef __cplusplus
}
#endif




template <class T>
T* sdf_get_value(std::map<std::string, T>* m, std::string name){
		if (m->count(name) > 0 ){
				return &(m->find(name)->second);
		}
		else{
				printf("\e[31mKey %s not found\e[0m\n", name.c_str());
				exit(EXIT_FAILURE);
		}
};

int stage_variables( SerialDataFormat* sdf ){
		std::vector<tagHeaderSDF> type_tags;
		if (sdf->vars_float == NULL || sdf->vars_int == NULL){
				return -1;
		}
		else{
			uint32_t count_size_for_header = 0;
			uint32_t number_of_features  = 0;
			for (auto it = sdf->vars_float->begin() ; it != sdf->vars_float->end(); ++it){
					tagHeaderSDF _type_tag = { it->first, FLOAT, 0};
					type_tags.push_back( _type_tag );
					sdf->buffer.push_back(memblockSDF() );
					count_size_for_header += sizeof(uint8_t) + it->first.size() + sizeof(FLOAT) + sizeof(uint32_t);
					number_of_features += 1;
			}
			for (auto it = sdf->vars_int->begin() ; it != sdf->vars_int->end(); ++it){
					tagHeaderSDF _type_tag = { it->first, INT, 0};
					type_tags.push_back( _type_tag );
					sdf->buffer.push_back(memblockSDF());
					count_size_for_header += sizeof(uint8_t) + it->first.size() + sizeof(INT) + sizeof(uint32_t);
					number_of_features += 1;
			}
			for (auto it = sdf->vars_str->begin() ; it != sdf->vars_str->end(); ++it){
					tagHeaderSDF _type_tag = { it->first, STR, 0};
					type_tags.push_back( _type_tag );
					sdf->buffer.push_back(memblockSDF());
					count_size_for_header += sizeof(uint8_t) + it->first.size() + sizeof(STR) + sizeof(uint32_t);
					number_of_features += 1;
			}
			sdf->header.type_tags = type_tags;
			sdf->header.start_buffer += count_size_for_header;
			sdf->header.number_of_features += number_of_features;
			return 0;
		}
}

template<typename T> 
std::array<uint8_t, sizeof(T)> to_bytes( const T* object )
{
    std::array< uint8_t, sizeof(T) > bytes ;

    const uint8_t* begin = reinterpret_cast< const uint8_t* >( object );
    const uint8_t* end = begin + sizeof(T) ;
    std::copy( begin, end, std::begin(bytes) ) ;

    return bytes ;
}



void update_buffers(SerialDataFormat* sdf){
		int enumerate_it = 0;
		sdf->header.number_of_events += 1;
		for (auto it = sdf->header.type_tags.begin(); it != sdf->header.type_tags.end(); it++){
				TypeTag type = it->type_info;
				std::string name = it->name;
				if ( type == FLOAT){
						auto bytes = to_bytes(sdf_get_value(sdf->vars_float, name));
						for (auto byte = bytes.begin(); byte != bytes.end(); byte++){
								sdf->buffer[enumerate_it].buffer.push_back(*byte); 
						}
				}
				else if ( type == INT){
						auto bytes = to_bytes(sdf_get_value(sdf->vars_int, name));
						for (auto byte = bytes.begin(); byte != bytes.end(); byte++){
								sdf->buffer[enumerate_it].buffer.push_back(*byte); 
						}
				}
				else if ( type == STR){
						std::string str			= *sdf_get_value(sdf->vars_str, name);
						stringSDF temp_str;	
						{	
								int i = 0;	
								for( auto it = str.begin(); it != str.end(); it++){
										if ( i > 255 ) break;		
										temp_str.characters[i] = *it;
										i++;
								}
                while(i < 256){
                    temp_str.characters[i] = char(0);
                    i++;
                }
						}
						
						auto bytes_str = to_bytes(&temp_str);

						for (auto byte = bytes_str.begin(); byte != bytes_str.end(); byte++){
								sdf->buffer[enumerate_it].buffer.push_back(*byte); 
						}
				}
				enumerate_it += 1;
		}
}
void write_sdf_to_disk(std::string file_name, SerialDataFormat* sdf ){
		std::fstream f;
		f.open(file_name.c_str(), std::fstream::out | std::fstream::binary);


		//NOTE
		//set buffer sizes
		{
				if(sdf->header.compression_flag == 0){
						for (unsigned int i = 0 ; i < sdf->buffer.size(); i++){
								sdf->header.type_tags[i].buffer_size = sdf->buffer[i].buffer.size();
						}
				}
				else{
						for (unsigned int i = 0 ; i < sdf->buffer.size(); i++){
								//CLEANUP 
								//We only want to do this alloc and compression one. 
								//We are wasting resources be running our compression algorithm multiple times.
								auto compression_len = compressBound(sdf->buffer[i].buffer.size());
								mz_uint8* mem_compressed = (mz_uint8 *)malloc((size_t)compression_len);
								compress(mem_compressed, &compression_len, (const unsigned char *)&sdf->buffer[i].buffer[0], sdf->buffer[i].buffer.size());

								sdf->header.type_tags[i].buffer_size = compression_len;
								free(mem_compressed);
						}
				}
		}



		////NOTE
		//write file header
		{
				f.write((char*)sdf->header.ID, sizeof(sdf->header.ID));	
				f.write((char*)&sdf->header.compression_flag, sizeof(sdf->header.compression_flag));	
				f.write((char*)&sdf->header.start_buffer, sizeof(sdf->header.start_buffer));	
				f.write((char*)&sdf->header.number_of_events, sizeof(sdf->header.number_of_events));	
				f.write((char*)&sdf->header.number_of_features, sizeof(sdf->header.number_of_features));	

				for (auto it = sdf->header.type_tags.begin(); it != sdf->header.type_tags.end(); it++){
						std::string name = it->name;
						TypeTag type = it->type_info;
						auto buffer_size = it->buffer_size;
						//Note:
						//Strings can only be 255 in length
						//And have zero termination character
						uint8_t string_length = (uint8_t) name.length();
						f.write((char*)&string_length, sizeof(uint8_t));
						f.write(name.c_str(), (size_t)string_length);
						f.write((char*)&type, sizeof(type));	
						f.write((char*)&buffer_size, sizeof(buffer_size));	
				}
        //NOTE
        //This is new. Needs to be tested.
        // -- TKG Dec 10, 2018
        f.write((char*)&sdf->header.notes.characters, sizeof(stringSDF));
		}

		////NOTE
		//write file data buffers
		{
				for (auto it = sdf->buffer.begin(); it != sdf->buffer.end(); it++){
						//CLEANUP:
						//You can do this in one go but I'm not sure how to handle sizeof
						if(sdf->header.compression_flag == 0){
								for (auto jt = it->buffer.begin(); jt != it->buffer.end(); jt++){
										uint8_t data = *jt;
										f.write((char*)&data, sizeof(data));	
								}
						}
						else{
								auto compression_len = compressBound(it->buffer.size());
								mz_uint8* mem_compressed = (mz_uint8 *)malloc((size_t)compression_len);
								int cmp_status = compress(mem_compressed, &compression_len, (const unsigned char *)&it->buffer[0], it->buffer.size());
								if(cmp_status) printf("Compression failed");

								f.write((const char *)mem_compressed, compression_len);	
								free(mem_compressed);
						}
				}
		}

		f.close();
}

headerSDF read_sdfheader_from_disk(std::fstream* f){
    headerSDF header;
    f->read((char*)header.ID, sizeof(header.ID));	
    if( header.ID[0] != 'T' || header.ID[1] != 'D' || header.ID[2] != 'F' ){
        _sdfERRNO = -1;
        return header;
    }
    f->read((char*)&header.compression_flag, sizeof(header.compression_flag));	
    f->read((char*)&header.start_buffer, sizeof(header.start_buffer));	
    f->read((char*)&header.number_of_events, sizeof(header.number_of_events));	
    f->read((char*)&header.number_of_features, sizeof(header.number_of_features));	


    //TODO
    //write into sdf struct
    std::vector<tagHeaderSDF> type_tags;
    for (uint32_t i = 0; i < header.number_of_features ; i++){
        TypeTag type; 
        //Note:
        //Strings can only be max 255 in length
        //And have NO zero termination character
        uint8_t string_length;
        f->read((char*)&string_length, sizeof(string_length));

        char* feature_name = (char*)malloc((size_t)string_length);	
        f->read(feature_name, (size_t)string_length);	

        std::string s = std::string(feature_name, string_length);
        f->read((char*)&type, sizeof(type));	

        
        uint32_t buffer_size = 0;
        f->read((char*)&buffer_size, sizeof(buffer_size));	

        tagHeaderSDF _type_tag = {s, type, buffer_size};
        type_tags.push_back(_type_tag);
        free(feature_name);
    }
    header.type_tags = type_tags;
    //NOTE
    //Needs to be tested.
    // --TKG Dec 10, 2018
    f->read((char*)&header.notes.characters, sizeof(stringSDF));	
    return header;
}

int get_tag_index(headerSDF* header, TypeTag type , std::string name){
    for (unsigned int i = 0; i < header->type_tags.size(); i++){
        if( header->type_tags[i].name == name  && header->type_tags[i].type_info == type) return i;
    }
    return 0;
}

std::vector<uint8_t> get_buffer_from_disk( std::fstream* f, tagHeaderSDF* tag, headerSDF* header){
    TypeTag type      = tag->type_info;
    std::string name  = tag->name;
    int type_tag_index = get_tag_index(header, type, name);
    uint64_t offset   = header->start_buffer ;

    for(unsigned int nth_feature = 0; nth_feature < header->type_tags.size(); nth_feature++){
        if ( header->type_tags[nth_feature].name == name ) break;
        offset += header->type_tags[nth_feature].buffer_size;
    }
    //TODO
    //Throw some sort of error it feature name is not found.


    std::vector<uint8_t> buffer;

    if (header->compression_flag == 0){
        if (type == FLOAT){
            float data;
            f->seekg(offset); 
            for (uint32_t i = 0; i != header->number_of_events; i++){
                f->read((char*)&data, sizeof(data));	
                auto bytes = to_bytes(&data);
                for (auto byte = bytes.begin(); byte != bytes.end(); byte++){
                    buffer.push_back(*byte); 
                }
            }
        }
        else if (type == INT){
            int32_t data;
            f->seekg(offset);
            for (uint32_t i = 0; i != header->number_of_events; i++){
                f->read((char*)&data, sizeof(data));	
                auto bytes = to_bytes(&data);
                for (auto byte = bytes.begin(); byte != bytes.end(); byte++){
                    buffer.push_back(*byte); 
                }
            }
        }
        else if (type == STR){
            stringSDF data;
            f->seekg(offset);
            for (uint32_t i = 0; i != header->number_of_events; i++){
                f->read((char*)&data, sizeof(data));	
                auto bytes = to_bytes(&data);
                for (auto byte = bytes.begin(); byte != bytes.end(); byte++){
                    buffer.push_back(*byte); 
                }
            }
        }
        else{
            printf("Error unknown type\n");
        }
    }
    else{ //TODO update to account for other compression flags like 16bit floats
        if (type == FLOAT){
            auto length_of_noncompressed_buffer = (size_t)header->number_of_events * sizeof(float);
            auto length_of_compressed_buffer = (size_t) header->type_tags[type_tag_index].buffer_size;

            mz_uint8* compressed_buffer = (mz_uint8 *)malloc(length_of_compressed_buffer);
            mz_uint8* noncompressed_buffer = (mz_uint8 *)malloc((size_t)length_of_noncompressed_buffer);

            f->seekg(offset);
            f->read((char *)compressed_buffer, length_of_compressed_buffer);	
            auto flag = uncompress(noncompressed_buffer, &length_of_noncompressed_buffer, compressed_buffer, length_of_compressed_buffer);
            if( flag != 0 ) printf("Decompression warning!!! \n Check decompression error code %d\n", flag);


            //NOTE:
            //I'm sure there is some more eff c++ thing 
            //can be done here
            for (unsigned int i = 0; i < length_of_noncompressed_buffer; i++){
                buffer.push_back( noncompressed_buffer[i] ); 
            }

            free(compressed_buffer);
            free(noncompressed_buffer);
        }
        else if (type == INT){
            auto length_of_noncompressed_buffer = (size_t)header->number_of_events * sizeof(int32_t);
            auto length_of_compressed_buffer = (size_t) header->type_tags[type_tag_index].buffer_size;

            mz_uint8* compressed_buffer = (mz_uint8 *)malloc(length_of_compressed_buffer);
            mz_uint8* noncompressed_buffer = (mz_uint8 *)malloc((size_t)length_of_noncompressed_buffer);

            f->seekg(offset);
            f->read((char *)compressed_buffer, length_of_compressed_buffer);	
            auto flag = uncompress(noncompressed_buffer, &length_of_noncompressed_buffer, compressed_buffer, length_of_compressed_buffer);
            if( flag != 0 ) printf("Decompression warning!!! \n Check decompression error code %d\n", flag);
            

            
            //NOTE:
            //I'm sure there is some more eff c++ thing 
            //can be done here
            for (unsigned int i = 0; i < length_of_noncompressed_buffer; i++){
                buffer.push_back(noncompressed_buffer[i]); 
            }

            free(compressed_buffer);
            free(noncompressed_buffer);

        }
        else if (type == STR){
                            
            auto length_of_noncompressed_buffer = (size_t)header->number_of_events * sizeof(stringSDF);
            auto length_of_compressed_buffer = (size_t) header->type_tags[type_tag_index].buffer_size;

            mz_uint8* compressed_buffer    = (mz_uint8 *)malloc(length_of_compressed_buffer);
            mz_uint8* noncompressed_buffer = (mz_uint8 *)malloc((size_t)length_of_noncompressed_buffer);

            f->seekg(offset);
            f->read((char *)compressed_buffer, length_of_compressed_buffer);	
            auto flag = uncompress(noncompressed_buffer, &length_of_noncompressed_buffer, compressed_buffer, length_of_compressed_buffer);
            if( flag != 0 ) printf("Check decompression!");
            

            //NOTE:
            //I'm sure there is some more eff c++ thing 
            //can be done here
            for (unsigned int i = 0; i < length_of_noncompressed_buffer; i++){
                buffer.push_back(noncompressed_buffer[i]); 
            }

            free(compressed_buffer);
            free(noncompressed_buffer);

        }
        else{
            printf("Error unknown type");
        }
    }
    return buffer;
}


SerialDataFormat read_sdf_from_disk(std::string file_name){
		std::fstream f;
		SerialDataFormat sdf;
		f.open(file_name.c_str(), std::fstream::in | std::fstream::binary);
		if(!f.good()){
				printf("File is note good!");
				return sdf;
		}

    sdf.header = read_sdfheader_from_disk(&f);
    if (_sdfERRNO != 0 ) return sdf; 
		//NOTE
		//read file data	
		{
				//Loop over features
				int enumerate_it = -1;	
				for(unsigned int i = 0; i != sdf.header.type_tags.size(); i++){

						enumerate_it += 1; 
						sdf.buffer.push_back(memblockSDF());
            sdf.buffer[enumerate_it].buffer = get_buffer_from_disk(&f, &sdf.header.type_tags[i], &sdf.header);
				}
		}
		
		f.close();
		return sdf;
}



//TEST
SerialDataFormat TEST_generateSDF(){
    SerialDataFormat sdf;
    std::map<std::string, float>        float_map;
    std::map<std::string, int>          int_map;
    std::map<std::string, std::string>  string_map;
  
    sdf.vars_float  = &float_map;
    sdf.vars_int    = &int_map;
    sdf.vars_str    = &string_map;

    float_map["grade"]  = 0.0;
    float_map["height(cm)"]  = 0.0;
    int_map["age"]      = 0;
    string_map["name"]  = "";

    stage_variables( &sdf );
    printf("variables staged\n");

    float grade_baseline[3] = {1.0, 0.8, 0.65 };
    std::string names[3] = {"Bob", "Matt", "Peter"};
    int cursor = 0;
    for( int i = 0 ; i < 10000; i++ ){
        float random = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) ;
        float_map["grade"] = (1.0 - random*random) * 100.0 * grade_baseline[cursor];
        float_map["height(cm)"] = (0.5 + 0.5*random*(random + 0.25 ) ) * 120.0;
        int_map["age"]     = 20 + i / 100;
        string_map["name"] = names[cursor];

        if( i < 10){
            printf( "grade %f\t\t", float_map["grade"]);
            printf( "height %f\t\t", float_map["height(cm)"]);
            printf( "age %d\t\t", int_map["age"]);
            printf( "name %s\n", string_map["name"].c_str());
        }

        cursor++;
        if(cursor > 2) cursor = 0;
        update_buffers( &sdf );
    }


    return sdf;  
}


int TEST_savetodisk(){
    SerialDataFormat sdf = TEST_generateSDF();

    sdf.header.compression_flag = 0;
    write_sdf_to_disk( "test_compression_0.sdf", &sdf );

    sdf.header.compression_flag = 1;
    write_sdf_to_disk( "test_compression_1.sdf", &sdf );
    return 0;  
}


int TEST_compare_written_vs_loaded_noncompressed(){
    SerialDataFormat sdf = TEST_generateSDF();

    sdf.header.compression_flag = 0;
    write_sdf_to_disk( "test_compression_0.sdf", &sdf );
    {
        SerialDataFormat sdf_1 = read_sdf_from_disk( "test_compression_0.sdf");
        if (sdf.buffer.size() != sdf_1.buffer.size()) printf("total number of buffers are not equal!!");
        for( unsigned int i = 0;  i < sdf.buffer.size(); i++){
            if(sdf.buffer[i].buffer.size() != sdf_1.buffer[i].buffer.size()){
                printf("feature \"%s\" does not have the same length between buffers! \t %lu %lu \n", sdf_1.header.type_tags[i].name.c_str(), sdf.buffer[i].buffer.size() , sdf_1.buffer[i].buffer.size());
            }
            for( int j = 0; j < 10; j++){
                printf("feature: %s orig: ", sdf_1.header.type_tags[i].name.c_str());
                if (sdf_1.header.type_tags[i].type_info == FLOAT){
                    printf(" %.2f next: %.2f\n", ((float *)&sdf.buffer[i].buffer[0])[j],  ((float *)&sdf_1.buffer[i].buffer[0])[j]);
                }
                else if (sdf_1.header.type_tags[i].type_info == INT){
                    printf(" %d next: %d\n", ((int *)&sdf.buffer[i].buffer[0])[j],  ((int *)&sdf_1.buffer[i].buffer[0])[j]);
                }
                else if (sdf_1.header.type_tags[i].type_info == STR){
                    printf(" %s next: %s\n", ((stringSDF *)&sdf.buffer[i].buffer[0])[j].characters,  ((stringSDF *)&sdf_1.buffer[i].buffer[0])[j].characters);
                }
                else{
                    printf("BE BETTER Type test has not been implemented.");
                }
            }
        }
        
    }
    return 0;
}

int TEST_compare_written_vs_loaded_compressed(){
    SerialDataFormat sdf = TEST_generateSDF();

    sdf.header.compression_flag = 1;
    write_sdf_to_disk( "test_compression_1.sdf", &sdf );
    {
        SerialDataFormat sdf_1 = read_sdf_from_disk( "test_compression_1.sdf");
        if (sdf.buffer.size() != sdf_1.buffer.size()) printf("total number of buffers are not equal!!");
        for( unsigned int i = 0;  i < sdf.buffer.size(); i++){
            if(sdf.buffer[i].buffer.size() != sdf_1.buffer[i].buffer.size()){
                printf("feature \"%s\" does not have the same length between buffers! \t %lu %lu \n", sdf_1.header.type_tags[i].name.c_str(), sdf.buffer[i].buffer.size() , sdf_1.buffer[i].buffer.size());
            }
            for( int j = 0; j < 10; j++){
                printf("feature: %s orig: ", sdf_1.header.type_tags[i].name.c_str());
                if (sdf_1.header.type_tags[i].type_info == FLOAT){
                    printf(" %.2f next: %.2f\n", ((float *)&sdf.buffer[i].buffer[0])[j],  ((float *)&sdf_1.buffer[i].buffer[0])[j]);
                }
                else if (sdf_1.header.type_tags[i].type_info == INT){
                    printf(" %d next: %d\n", ((int *)&sdf.buffer[i].buffer[0])[j],  ((int *)&sdf_1.buffer[i].buffer[0])[j]);
                }
                else if (sdf_1.header.type_tags[i].type_info == STR){
                    printf(" %s next: %s\n", ((stringSDF *)&sdf.buffer[i].buffer[0])[j].characters,  ((stringSDF *)&sdf_1.buffer[i].buffer[0])[j].characters);
                }
                else{
                    printf("BE BETTER Type test has not been implemented.");
                }
            }
        }
    }
    printf("test complete\n");
    return 0;
}


void TEST_notes_generation(){
    SerialDataFormat sdf = TEST_generateSDF();
    strcpy(sdf.header.notes.characters, "This is a test of the notes buffer. Yacady sax");
    write_sdf_to_disk( "test_notes.sdf", &sdf );
}






