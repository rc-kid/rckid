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



 */
#include <rckid/stream.h>

inline Writer operator << (Writer writer, float value) {
    UNIMPLEMENTED;
    return writer;
}


namespace rckid {

    class Serializer {
    public:
        Serializer(WriteStream & stream): 
            stream_{stream} {
        }
        virtual ~Serializer() = default;

        virtual void serializeChar(char what) = 0;
        virtual void serializeString(String const & what) = 0;
        virtual void serializeUint8(uint8_t what) = 0;
        virtual void serializeUint16(uint16_t what) = 0;
        virtual void serializeUint32(uint32_t what) = 0;
        virtual void serializeUint64(uint64_t what) = 0;
        virtual void serializeInt8(int8_t what) = 0;
        virtual void serializeInt16(int16_t what) = 0;
        virtual void serializeInt32(int32_t what) = 0;
        virtual void serializeInt64(int64_t what) = 0;
        virtual void serializeBool(bool what) = 0;
        virtual void serializeFloat(float what) = 0;
    
    protected:
        WriteStream & stream_;
    }; 

    class Deserializer {
        virtual ~Deserializer() = default;

        virtual char deserializeChar() = 0;
        virtual String deserializeString() = 0;
        virtual uint8_t deserializeUint8() = 0;
        virtual uint16_t deserializeUint16() = 0;
        virtual uint32_t deserializeUint32() = 0;
        virtual uint64_t deserializeUint64() = 0;
        virtual int8_t deserializeInt8() = 0;
        virtual int16_t deserializeInt16() = 0;
        virtual int32_t deserializeInt32() = 0;
        virtual int64_t deserializeInt64() = 0;
        virtual bool deserializeBool() = 0;
        virtual float deserializeFloat() = 0;
    }; 

    inline Serializer & operator << (Serializer & s, char what) {
        s.serializeChar(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, String const & what) {
        s.serializeString(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, char const * str) {
        s.serializeString(String{str});
        return s;
    }

    inline Serializer & operator << (Serializer & s, uint8_t what) {
        s.serializeUint8(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, uint16_t what) {
        s.serializeUint16(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, uint32_t what) {
        s.serializeUint32(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, uint64_t what) {
        s.serializeUint64(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, int8_t what) {
        s.serializeInt8(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, int16_t what) {
        s.serializeInt16(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, int32_t what) {
        s.serializeInt32(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, int64_t what) {
        s.serializeInt64(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, bool what) {
        s.serializeBool(what);
        return s;
    }

    inline Serializer & operator << (Serializer & s, float what) {
        s.serializeFloat(what);
        return s;
    }

    /** Serializes data in binary format. 
     */
    class BinarySerializer : public Serializer {
    public:
        BinarySerializer(WriteStream & stream):
            Serializer{stream} {
        }
        
        void serializeChar(char what) override { stream_.writeByte(static_cast<uint8_t>(what)); }
        
        void serializeString(String const & what) override {   
            uint32_t size = what.size();
            serializeUint32(size);
            stream_.write(reinterpret_cast<uint8_t const*>(what.c_str()), size);
        }
        
        void serializeUint8(uint8_t what) override { stream_.writeByte(what); }
        
        void serializeUint16(uint16_t what) override { 
            stream_.writeByte(static_cast<uint8_t>(what & 0xff));
            stream_.writeByte(static_cast<uint8_t>(what >> 8)); 
        }

        void serializeUint32(uint32_t what) override { 
            serializeUint16(static_cast<uint16_t>(what & 0xffff));
            serializeUint16(static_cast<uint16_t>(what >> 16));
        }

        void serializeUint64(uint64_t what) override { 
            serializeUint32(static_cast<uint32_t>(what & 0xffffffff));
            serializeUint32(static_cast<uint32_t>(what >> 32));
        }

        void serializeInt8(int8_t what) override { serializeUint8(static_cast<uint8_t>(what)); }
        void serializeInt16(int16_t what) override { serializeUint16(static_cast<uint16_t>(what)); }
        void serializeInt32(int32_t what) override { serializeUint32(static_cast<uint32_t>(what)); }
        void serializeInt64(int64_t what) override { serializeUint64(static_cast<uint64_t>(what)); }
        void serializeBool(bool what) override { what ? serializeUint8(1) : serializeUint8(0); }
        void serializeFloat(float what) override { 
            stream_.write(reinterpret_cast<uint8_t const *>(& what), sizeof(float));
        }
    }; 

    /** Serializes data in human readable format. 
     
     */
    class TextSerializer : public Serializer {
    public:
        TextSerializer(WriteStream & stream):
            Serializer{stream} {
        }
        void serializeChar(char what) override { stream_.writer() << what; }
        void serializeString(String const & what) override { stream_.writer() << what.c_str(); }
        void serializeUint8(uint8_t what) override { stream_.writer() << what; }
        void serializeUint16(uint16_t what) override { stream_.writer() << what; }
        void serializeUint32(uint32_t what) override { stream_.writer() << what; }
        void serializeUint64(uint64_t what) override { stream_.writer() << what; }
        void serializeInt8(int8_t what) override { stream_.writer() << what; }
        void serializeInt16(int16_t what) override { stream_.writer() << what; }
        void serializeInt32(int32_t what) override { stream_.writer() << what; }
        void serializeInt64(int64_t what) override { stream_.writer() << what; }
        void serializeBool(bool what) override { stream_.writer() << (what ? "true" : "false"); }
        void serializeFloat(float what) override { stream_.writer() << what; }
    }; 


} // namespace rckid