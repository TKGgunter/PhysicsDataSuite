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
    type_info = 0
    buffer_size = 0


SDF_FLOAT = 0
SDF_INT = 1
SDF_STR = 2
#TODO make enum
#enum TypeTag{
#	FLOAT = 0,
#	INT,
#	STR 
#};


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
    if header.compression_flag != 0:
        if tag.type_info == 0 or tag.type_info == 1: 
            membuffer._buffer = zlib.decompress(membuffer._buffer, bufsize=header.number_of_events * 4) 
        elif tag.type_info == 2:
            membuffer._buffer = zlib.decompress(membuffer._buffer, bufsize=header.number_of_events * 256) 

    return membuffer


def SDFmembuffer_to_array(membuffer, sdf_type):
    if sdf_type < 2:
        np_type = numpy.float32
        if sdf_type == 1:
            np_type = numpy.int32
        return numpy.fromstring(membuffer._buffer, np_type)
    else:
        return numpy.array(([membuffer._buffer[i:i+256].decode("utf-8") for i in range(0, len(membuffer._buffer), 256 )]))

#TODO
#Test Me !!!
def SDFtoPandas(filename, columns=None):
    import pandas as pd
    f = SDFopen(filename)
    header = SDFloadHeader(f)

    if type(column) == type(None):
        column = []
        for tt in header.type_tags:
            column.append(tt.name)


    #NOTE
    #loop through header of SDF file to determine is all elements of column are in the file
    map_c = {i:False for i in columns}
    map_c_tt = {i:SDF_FLOAT for i in columns}
    for c in columns: 
        for tt in header.type_tags
            if c == tt.name:
                map_c[c] = True
                map_tt[c] = tt.type_info
                continue

    dic_predf = {}
    for c in map_c:
        if map_c[c] != True:
           #NOTE
           #Idk if an exception is what I want to use might change later
           # -- TKG Dec 8, 2018
            raise Exception(c + " Not found in file")
        else:
            _buffer = SDFload_buffer(f, header, c)
            dic_predf[c] = SDFmembuffer_to_array(a, map_tt[c])
    df = pd.DataFrame(dic_predf)

    f.close()
    return df

#TODO
#Test Me !!!
def SDFaddtoPandas( df, filename, column ):
    import pandas as pd
    f = SDFopen(filename)
    header = SDFloadHeader(f)
    
    for tt in header.type_tags
        if column == tt.name:
            _buffer = SDFload_buffer(f, header, column)
            _array = SDFmembuffer_to_array(a, tt.type_info)
            break
    #Note
    #helpful hint maybe
    #https://stackoverflow.com/questions/12555323/adding-new-column-to-existing-dataframe-in-python-pandas
    df = df.assign(column=p.Series( _array ).values)

    f.close()
    return df 



if __name__ == "__main__":
    f = SDFopen("test_compression_0.sdf")
    header = SDFloadHeader(f)
    a = SDFload_buffer(f, header, b'name')
    print("n events", header.number_of_events)
    b = SDFmembuffer_to_array(a, SDF_STR)
    print( b )






