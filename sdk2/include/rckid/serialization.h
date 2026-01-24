#pragma once


/** Serialization & Deserialization
 
    RCKid SDK provides basic serialization primitives. This


    So serialize can ingest types, in which case it binary serializes them, it can also ingest section and field, in which case it can do fancy things

    serializer
        << Section("foo")
        << Field("bar", 67)
        << Field("baz", 67)


    deserializer
        >> Section("foo")
        >> Field("bar", someValue)
        >> Field("baz", someValue)

    Binary serializer will ignore section 

    Have BinarySerializer, TextSerializer, INISerializer, etc. They all take & use stream

    Serializers take over stream, then have virtual methods for serializing primitive types, sections, fields, etc. 

    operator << and >> will call those methods. Users can create their own operators for their own types that break them down into primitive types that serializers understand. 

    Fields and Sections are special and serializers understand & react to them. 

    Or maybe do *not* take over the stream, just reference it? Much simpler design, all it costs me is extra line for first have stream, then create serializer with it


    INI is the most painful, for that we can use 





    serializer << Section("name, {
        Field("age", 67);
        Field("name", "Peta");
    });")

    deserializer >> Section("name", {
        Field("age", age);
        Field("name", name);
    });")



 */

namespace rckid {

    /**  */
    class Reader {
    public:
        using GetCharCallback = std::function<int32_t(bool)>;

        explicit Reader(GetCharCallback getChar): 
            getChar_{getChar} {
        }

        char getChar() {
            int result = rawCallback(true);
            if (result < 0)
              result = 0;
            return static_cast<char>(result);
        }

        char peekChar() {
            int result = rawCallback(false);
            if (result < 0)
              result = 0;
            return static_cast<char>(result);
        }

        bool eof() {
            return rawCallback(false) < 0;
        }

        int32_t rawCallback(bool advance = true) { return getChar_(advance); }

    private:
        GetCharCallback getChar_;
    }; 

    inline Reader operator >> (Reader r, char & c) {
        c = r.getChar();
        return r;
    }

    // TODO do others
    

    /** Similar to platform writer,  */
    class BinaryWriter {

    }; // BinaryWriter

    class BinaryReader {

    }; 


} // namespace rckid


