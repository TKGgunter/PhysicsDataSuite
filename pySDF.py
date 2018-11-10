#Thoth Gunter
import numpy
import zlib



class headerSDF:
    ID = b'0'
    compression_flag = -1
    start_buffer = -1
    number_of_events = -1
    type_tags = [] 

#struct tagHeaderSDF{
#		std::string name;
#		TypeTag type_info;
#		uint32_t buffer_size;
#};

class tagHeaderSDF:
    name = ""
    type_info = 0 #TODO make enum
    buffer_size = 0

def SDFopen(file_name):
    f = open(file_name, "rb")
    header = f.read(3)
    if header != b'TDF':
        print("File is not a SDF!")
        return -1
    else:
        f.seek(0)
        return f

def SDFloadHeader(f):
    header = headerSDF()
    header.ID = f.read(3)
    header.compression_flag = int.from_bytes(f.read(1), byteorder='little', signed=False)
    header.start_buffer =  int.from_bytes(f.read(4), byteorder='little', signed=False)
    header.number_of_events =  int.from_bytes(f.read(4), byteorder='little', signed=False)
    header.number_of_features =  int.from_bytes(f.read(4), byteorder='little', signed=False)
   
    for i in range(header.number_of_features):
        tag = tagHeaderSDF()
        name_length = int.from_bytes( f.read(1), byteorder='little', signed=False)
        tag.name = f.read(name_length)
        tag.type_info   = int.from_bytes( f.read(4), byteorder='little', signed=False)
        tag.buffer_size = int.from_bytes( f.read(4), byteorder='little', signed=False)

        header.type_tags.append(tag)

    return header

def SDFload_buffer(f, header, key):
    return "POOP"






















