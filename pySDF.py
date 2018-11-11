#Thoth Gunter
import numpy
import zlib



class headerSDF:
    ID = b'0'
    compression_flag = -1
    start_buffer = -1
    number_of_events = -1
    type_tags = [] 


class tagHeaderSDF:
    name = ""
    type_info = 0 #TODO make enum
    buffer_size = 0



class memblockSDF:
    _buffer = b''

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
    if type(key) == str :
        key = bytes(key, 'utf-8')

    membuffer = memblockSDF()
    _index = -1
    bytes_before = header.start_buffer

    buffer_size = 0
    type_info = 0

    for i, tag in enumerate(header.type_tags):
        print( bytes_before )
        if _index != -1:
            break
        if tag.name == key:
            _index = i
            buffer_size = tag.buffer_size
            type_info   = tag.type_info
            break
        else:
            bytes_before += tag.buffer_size
    
    
    f.seek(bytes_before)
    membuffer._buffer = f.read(buffer_size)
    if header.compression_flag == 1:
        #TODO
        #We only take care to INTS and FLOATS right now
        if 
        membuffer._buffer = zlib.decompress(membuffer._buffer, bufsize=header.number_of_events * 4) 
    return membuffer


def SDFmembuffer_to_array(membuffer, np_type):
    return numpy.fromstring(membuffer._buffer, np_type)


if __name__ == "__main__":
    f = SDFopen("test_compression_1.sdf")
    header = SDFloadHeader(f)
    a = SDFload_buffer(f, header, b'grade')
    print("n events", header.number_of_events)
    b = SDFmembuffer_to_array(a, numpy.float32)
    print( b )














