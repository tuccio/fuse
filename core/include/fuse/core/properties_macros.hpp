#pragma once

#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/seq.hpp>

#include <type_traits>

#define FUSE_PROPERTY_GET_BY_CONST_REFERENCE(PropertyName, MemberVariable) BOOST_PP_CAT(inline auto get_, PropertyName (void) const -> decltype((MemberVariable)) { return MemberVariable ; })
#define FUSE_PROPERTY_SET_BY_CONST_REFERENCE(PropertyName, MemberVariable) BOOST_PP_CAT(inline void set_, PropertyName (const decltype(MemberVariable) & value) { MemberVariable = value; })

#define FUSE_PROPERTY_GET_BY_VALUE(PropertyName, MemberVariable) BOOST_PP_CAT(inline auto get_, PropertyName (void) const -> std::remove_const<decltype(MemberVariable)>::type { return MemberVariable ; })
#define FUSE_PROPERTY_SET_BY_VALUE(PropertyName, MemberVariable) BOOST_PP_CAT(inline void set_, PropertyName (std::remove_const<decltype(MemberVariable)>::type value) { MemberVariable = value; })

#define FUSE_PROPERTY_GET_SMART_POINTER(PropertyName, MemberVariable) BOOST_PP_CAT(inline auto get_, PropertyName (void) const -> decltype(MemberVariable.get()) { return MemberVariable ## .get(); })
#define FUSE_PROPERTY_SET_SMART_POINTER(PropertyName, MemberVariable) BOOST_PP_CAT(inline void set_, PropertyName (decltype(MemberVariable.get()) value) { MemberVariable = value; })

#define FUSE_PROPERTY_GET_STRING(PropertyName, MemberVariable) BOOST_PP_CAT(inline decltype(auto) get_, PropertyName (void) { return MemberVariable.c_str(); })
#define FUSE_PROPERTY_SET_STRING(PropertyName, MemberVariable) BOOST_PP_CAT(inline void set_, PropertyName (const decltype(MemberVariable)::const_pointer value) { MemberVariable = value; })

#define FUSE_PROPERTY_BY_CONST_REFERENCE(PropertyName, MemberVariable) FUSE_PROPERTY_GET_BY_CONST_REFERENCE(PropertyName, MemberVariable) FUSE_PROPERTY_SET_BY_CONST_REFERENCE(PropertyName, MemberVariable)
#define FUSE_PROPERTY_BY_VALUE(PropertyName, MemberVariable) FUSE_PROPERTY_GET_BY_VALUE(PropertyName, MemberVariable) FUSE_PROPERTY_SET_BY_VALUE(PropertyName, MemberVariable)
#define FUSE_PROPERTY_STRING(PropertyName, MemberVariable) FUSE_PROPERTY_GET_STRING(PropertyName, MemberVariable) FUSE_PROPERTY_SET_STRING(PropertyName, MemberVariable)
#define FUSE_PROPERTY_SMART_POINTER(PropertyName, MemberVariable) FUSE_PROPERTY_GET_SMART_POINTER(PropertyName, MemberVariable) FUSE_PROPERTY_SET_SMART_POINTER(PropertyName, MemberVariable)

#define FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR1(A, B) ((A, B)) FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR2
#define FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR2(A, B) ((A, B)) FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR1
#define FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR1_END
#define FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR2_END
#define FUSE_MAKE_SEQUENCE_OF_TUPLES(Input) BOOST_PP_CAT(FUSE_MAKE_SEQUENCE_OF_TUPLES_PAR1 Input, _END) 

#define FUSE_MAKE_PROPERTY_BY_CONST_REFERENCE(Size, Data, Element) FUSE_PROPERTY_BY_CONST_REFERENCE(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))
#define FUSE_MAKE_PROPERTY_BY_VALUE(Size, Data, Element) FUSE_PROPERTY_BY_VALUE(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))
#define FUSE_MAKE_PROPERTY_STRING(Size, Data, Element) FUSE_PROPERTY_STRING(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))
#define FUSE_MAKE_PROPERTY_SMART_POINTER(Size, Data, Element) FUSE_PROPERTY_SMART_POINTER(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))

#define FUSE_MAKE_PROPERTY_BY_CONST_REFERENCE_READ_ONLY(Size, Data, Element) FUSE_PROPERTY_GET_BY_CONST_REFERENCE(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))
#define FUSE_MAKE_PROPERTY_BY_VALUE_READ_ONLY(Size, Data, Element) FUSE_PROPERTY_GET_BY_VALUE(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))
#define FUSE_MAKE_PROPERTY_SMART_POINTER_READ_ONLY(Size, Data, Element) FUSE_PROPERTY_GET_SMART_POINTER(BOOST_PP_TUPLE_ELEM(2, 0, Element), BOOST_PP_TUPLE_ELEM(2, 1, Element))

#define FUSE_PROPERTIES_BY_CONST_REFERENCE(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_BY_CONST_REFERENCE, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))
#define FUSE_PROPERTIES_BY_VALUE(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_BY_VALUE, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))
#define FUSE_PROPERTIES_STRING(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_STRING, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))

#define FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_BY_CONST_REFERENCE_READ_ONLY, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))
#define FUSE_PROPERTIES_BY_VALUE_READ_ONLY(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_BY_VALUE_READ_ONLY, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))
#define FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(Properties) BOOST_PP_SEQ_FOR_EACH(FUSE_MAKE_PROPERTY_SMART_POINTER_READ_ONLY, 0, FUSE_MAKE_SEQUENCE_OF_TUPLES(Properties))