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

 */

namespace rckid {

    /** Field serializer.
     */
    template<typename T>
    auto Field(T const & value) {
        return [=]() {

        }; 
    } 

} // namespace rckid