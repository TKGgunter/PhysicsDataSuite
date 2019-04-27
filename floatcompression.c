//TODO
// Make thing more C-like to ease use with other languages
// + 16bit float


//NOTE
//dll compile
//  g++ floatcompression.c -o floatcompression -lX11 -std=c++11 && ./floatcompression

#include "half.c"
#include "miniz.h"
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <stdlib.h>     /* exit, EXIT_FAILURE */

//TODO
//+ add half bit float 


struct compressedKey{
    float main_binnings[256];
};
struct compressedFloat{
    uint8_t bin0;
    uint8_t bin1;
};


compressedKey construct_compressed_key( std::vector<float> arr ){
    float min = 0;
    float max = 0;

    for(int i = 0; i < arr.size(); i++){
        if( i == 0 ) {
            arr[i];
            arr[i];
        }
        if( arr[i] < min ) min = arr[i];
        if( arr[i] > max ) max = arr[i];
    }

    compressedKey  cmkey;
    float range = max - min;
    for(int i = 0; i < 256; i++){
        cmkey.main_binnings[i] = float(i)/255. * range + min;
    }

    return cmkey;
}


int float_to_compressed( float in, compressedFloat* out, compressedKey* cmkey ){
    for(int i = 0; i < 255; i++){
        if( in >= cmkey->main_binnings[i] && in <= cmkey->main_binnings[i+1]){
            for(int j = 0; j < 255; j++){
                float _in = in - cmkey->main_binnings[i];
                float _range = cmkey->main_binnings[i+1] - cmkey->main_binnings[i];
                if ( _in >= _range*float(j) / 255. &&  _in <= _range*float(j+1)/255.){
                    out->bin0 = i; 
                    out->bin1 = j;
                    return 0;
                }
            }
        } 
    }
    return -1;
}

int compressed_to_float(compressedFloat in, float* out, compressedKey* cmkey ){
    if (in.bin0 > 255 || in.bin1 > 255) return -1;
    float range  = cmkey->main_binnings[in.bin0+1] - cmkey->main_binnings[in.bin0];
    *out = cmkey->main_binnings[in.bin0] + float(in.bin1)/ 255. * range;
}

//TODO
int floatbuffer_to_cmpfloatbuffer(){
    return 0;
}

//TODO
int cmpfloatbuffer_to_floatbuffer(){
    return 0;
}

void test_write(int n_randoms);
void test_read(int n_randoms);

union FLOAT_32{
    float    f32;
    uint32_t u32;
};


int main (int n_command_line_args, char** argv) {
    /*
    std::vector<float> arr;

    int i=0;
    while (i < 10000){ 
        i += 1;
        float random = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        arr.push_back(random);

    } 

    compressedKey key = construct_compressed_key( arr );
    float mean_residual = 0;
    i = 0;
    while (i < 10000){ 
        compressedFloat cmpfloat;
        float_to_compressed( arr[i], &cmpfloat, &key);
        float f;
        compressed_to_float(cmpfloat, &f, &key);
        mean_residual += fabs(arr[i] - f);
        i++;
    }
    mean_residual /= 10000.0;

    printf("%f",mean_residual);
*/

    union FLOAT_32 _a = {.f32 = 32.32};
    auto a = half_from_float( _a.u32);
    int r = 1e7 + 0;
    //test_write( r );
    test_read( r );
    return 0;
}


template<typename T> 
std::array<uint8_t, sizeof(T)> to_bytes( const T* object )
{
    std::array< uint8_t, sizeof(T) > bytes ;

    const uint8_t* begin = reinterpret_cast< const uint8_t* >( object );
    const uint8_t* end = begin + sizeof(T) ;
    std::copy( begin, end, std::begin(bytes) ) ;

    return bytes;
}



void test_write(int n_randoms){
    //NOTE
    //We compare the non-compressed float, non-compressed tkgcompress, zipped float, zipped tkgcompressed, zfp float.
    // we need compression timing info, compression ratio, decompression timing info

    std::vector<float> arr;
    int i=0;
    while (i < n_randoms){ 
        i += 1;
        float random = powf((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), 2.0);
        arr.push_back(random);

    } 

    
    //write float file
    {
        clock_t start = clock();

		std::fstream f;
		f.open("float_compression/float.tmp", std::fstream::out | std::fstream::binary);
		f.write((const char*)&arr[0], sizeof(float)*arr.size()); //CHECK
		f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write float file duration: %f seconds\n", duration);
    }
    
    //write compressed_float file
    {
        clock_t start = clock();

        compressedKey key = construct_compressed_key( arr );
        std::vector<compressedFloat> cmp_arr(n_randoms); 
        float mean_residual = 0;
        i = 0;
        while (i < n_randoms){ 
            compressedFloat cmpfloat;
            float_to_compressed( arr[i], &cmpfloat, &key);
            cmp_arr[i] = cmpfloat;
            i++;
        }
		std::fstream f;
		f.open("float_compression/compressed_float.tmp", std::fstream::out | std::fstream::binary);
		f.write((const char*) &key, sizeof(key));
		f.write((const char*)&cmp_arr[0], sizeof(compressedFloat)*cmp_arr.size());
		f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write histogram compressed float file duration: %f seconds\n", duration);
    }
 
    
    //write float zipped file
    {
        clock_t start = clock();

        auto compression_len = compressBound(sizeof(float)*arr.size());
        mz_uint8* mem_compressed = (mz_uint8 *)malloc((size_t)compression_len);
        int cmp_status = compress(mem_compressed, &compression_len, (const unsigned char *)&arr[0], sizeof(float)*arr.size());
        if(cmp_status) printf("Compression failed");

		std::fstream f;
		f.open("float_compression/float_zip.tmp", std::fstream::out | std::fstream::binary);
        f.write((const char *)mem_compressed, compression_len);	
        free(mem_compressed);
        f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write zipped float file duration: %f seconds\n", duration);
    }


    
    //write compress_float zipped file
    {
        clock_t start = clock();

        compressedKey key = construct_compressed_key(arr);
        std::vector<compressedFloat> cmp_arr(n_randoms); 
        float mean_residual = 0;
        i = 0;
        while (i < n_randoms){ 
            compressedFloat cmpfloat;
            if ( float_to_compressed( arr[i], &cmpfloat, &key) != 0 ) 
                printf("Something is wrong!! %d %f\n", i, arr[i]);
            cmp_arr[i] = cmpfloat;
            i++;
        }
        auto compression_len = compressBound(cmp_arr.size()*sizeof(compressedFloat));
        mz_uint8* mem_compressed = (mz_uint8 *)malloc((size_t)compression_len);
        int cmp_status = compress(mem_compressed, &compression_len, (const unsigned char *)&cmp_arr[0], cmp_arr.size()*sizeof(compressedFloat));
        if(cmp_status) printf("Compression failed");

        //size_t len = sizeof(compressedFloat)*n_randoms;
        //std::vector<compressedFloat> cmpfloat_arr(n_randoms);
        //auto flag = uncompress((unsigned char*)&cmpfloat_arr[0], &len, mem_compressed, compression_len);
        //printf("post compress decompress %d %d %d\n", flag, compression_len, len);

		std::fstream f;
		f.open("float_compression/compressed_float_zip.tmp", std::fstream::out | std::fstream::binary);
		f.write((const char*) &key, sizeof(key));
        f.write((const char *)mem_compressed, compression_len);	
        free(mem_compressed);
        f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write zipped histogram float file duration: %f seconds\n", duration);
    }

    {//half  float
        clock_t start = clock();

        std::vector<uint16_t> cmp_arr(n_randoms); 
        i = 0;
        while (i < n_randoms){ 
            union FLOAT_32 _f = {.f32 = arr[i]};
            cmp_arr[i] = half_from_float(_f.u32);
            union FLOAT_32 _g = {.u32 = half_to_float(cmp_arr[i])};
            i++;
        }

		std::fstream f;
		f.open("float_compression/half.tmp", std::fstream::out | std::fstream::binary);
		f.write((const char*)&cmp_arr[0], sizeof(float)*cmp_arr.size()); //CHECK
		f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write half float file duration: %f seconds\n", duration);
    }

    {//half float zip
        clock_t start = clock();

        std::vector<uint16_t> cmp_arr(n_randoms); 
        i = 0;
        while (i < n_randoms){ 
            union FLOAT_32 _f = {.f32 = arr[i]};
            cmp_arr[i] = half_from_float(_f.u32);
            i++;
        }

        auto compression_len = compressBound(cmp_arr.size()*sizeof(uint16_t));
        mz_uint8* mem_compressed = (mz_uint8 *)malloc((size_t)compression_len);
        int cmp_status = compress(mem_compressed, &compression_len, (const unsigned char *)&cmp_arr[0], cmp_arr.size()*sizeof(uint16_t));
        if(cmp_status) printf("Compression failed");


		std::fstream f;
		f.open("float_compression/half_zip.tmp", std::fstream::out | std::fstream::binary);
		f.write((const char*)&mem_compressed[0], compression_len); //CHECK
		f.close();

        float duration = (clock() - start) / (float) CLOCKS_PER_SEC;
        printf("write half float zip file duration: %f seconds\n", duration);
    }
    
}


void test_read(int n_randoms){
    printf("\n\n");

    std::vector<float> arr(n_randoms);
    {//Read binary float files
		std::fstream f;
		f.open("float_compression/float.tmp", std::fstream::in | std::fstream::binary);
        f.read((char*)&arr[0], sizeof(float)*n_randoms);	
        f.close();
    }

    std::vector<float> convert_float_arr(n_randoms);
    {//Read binary cmpfloat files
		std::fstream f;
		f.open("float_compression/compressed_float.tmp", std::fstream::in | std::fstream::binary);

        compressedKey key;
        std::vector<compressedFloat> cmpfloat_arr(n_randoms);
        f.read((char*)&key, sizeof(compressedKey));	
        f.read((char*)&cmpfloat_arr[0], sizeof(compressedFloat)*n_randoms);	

        float mean_residual = 0;
        int i=0;
        while( i < n_randoms){
            float _f;
            compressed_to_float(cmpfloat_arr[i], &_f, &key );
            convert_float_arr.push_back(_f); 

            //printf("%f %f\n",  );
            mean_residual += fabs( _f - arr[i]);

            i++;
        }
        printf("mean residual binary float read %f\n", mean_residual/n_randoms );

    }

    
    {//Read binary zipfloat files
		std::fstream f;
		f.open("float_compression/float_zip.tmp", std::fstream::in | std::fstream::binary);

        std::vector<float> cmp_arr(n_randoms);
        std::vector<float> decmp_arr(n_randoms);

        f.read((char *)&cmp_arr[0], sizeof(float)*n_randoms);
        f.close();

        size_t len = sizeof(float)*n_randoms;
        auto flag = uncompress((unsigned char *)&decmp_arr[0], &len, (unsigned char *)&cmp_arr[0], sizeof(float)*n_randoms);
        if( flag != 0 ) printf("Decompression warning!!! \n Check decompression error code %d\n", flag);

        float mean_residual = 0;
        for(int i = 0; i < n_randoms; i++){
            mean_residual += fabs( decmp_arr[i] - arr[i]);
        }
        printf("mean residual binary zip compressed read %f\n", mean_residual/n_randoms );
    }

    {//Read binary zipcmpfloat files
		std::fstream f;
		f.open("float_compression/compressed_float_zip.tmp", std::fstream::in | std::fstream::binary);
        compressedKey key;
        f.read((char*)&key, sizeof(compressedKey));	

        std::vector<compressedFloat> cmpfloat_arr(n_randoms);
        std::vector<compressedFloat> cmp_arr(n_randoms);
        std::vector<float> decmp_arr(n_randoms);

        f.read((char *)&cmp_arr[0], sizeof(compressedFloat)*n_randoms);
        f.close();

        auto len = compressBound(n_randoms*sizeof(compressedFloat));
        auto flag = uncompress((unsigned char *)&cmpfloat_arr[0], &len, (unsigned char *)&cmp_arr[0], sizeof(compressedFloat)*n_randoms);
        if( flag != 0 ) printf("ADSFASDF Decompression warning!!! \n Check decompression error code %d\n", flag);

        float mean_residual = 0;
        for(int i = 0; i < n_randoms; i++){
            float _f;
            compressed_to_float(cmpfloat_arr[i], &_f, &key );
            decmp_arr[i] = _f; 
            mean_residual += fabs( _f - arr[i] );
            if (_f == 0.0 && arr[i] >= 0.01){
                printf("%d %f %f\n", i,  _f, arr[i]);
                break;
            }
        }
        printf("mean residual binary histogram zip float %f\n", mean_residual/n_randoms );
    }
    {//Read binary half float files
		std::fstream f;
        std::vector<uint16_t> cmp_arr(n_randoms);
		f.open("float_compression/half.tmp", std::fstream::in | std::fstream::binary);
        f.read((char*)&cmp_arr[0], sizeof(uint16_t)*n_randoms);	
        f.close();

        float mean_residual = 0;
        for(int i = 0; i < n_randoms; i++){
            union FLOAT_32 _f;
            _f.u32 = half_to_float(cmp_arr[i]);
            mean_residual += fabs( _f.f32 - arr[i] );
            if (_f.f32 == 0.0 && arr[i] >= 0.01){
                printf("%d %f %f\n", i,  _f.f32, arr[i]);
                break;
            }
        }
        printf("mean residual binary half float %f\n", mean_residual/n_randoms );
    }


    {//Read binary half float zip files
		std::fstream f;
        std::vector<uint16_t> cmp_arr(n_randoms);
		f.open("float_compression/half_zip.tmp", std::fstream::in | std::fstream::binary);
        f.read((char*)&cmp_arr[0], sizeof(uint16_t)*n_randoms);	
        f.close();

        auto len = compressBound(n_randoms*sizeof(uint16_t));
        std::vector<uint16_t> cmpfloat_arr(len);
        auto flag = uncompress((unsigned char *)&cmpfloat_arr[0], &len, (unsigned char *)&cmp_arr[0], sizeof(uint16_t)*n_randoms);
        if (flag != 0) printf("uncompress issue %d", flag);

        float mean_residual = 0;
        for(int i = 0; i < n_randoms; i++){
            union FLOAT_32 _f;
            _f.u32 = half_to_float(cmpfloat_arr[i]);
            mean_residual += fabs( _f.f32 - arr[i] );
            if (_f.f32 == 0.0 && arr[i] >= 0.01){
                printf("%d %f %f\n", i,  _f.f32, arr[i]);
                break;
            }
        }
        printf("mean residual binary half float zip %f\n", mean_residual/n_randoms );
    }
    
}
