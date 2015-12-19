#ifndef _CCBII_MAPPING_H_
#define _CCBII_MAPPING_H_

/**
* @brief the tag string about ccbi xml
*/
#define CCBI_XML_DECLARATION "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
	"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
#define CCBI_XML_ROOT_START_PART "<plist version=\"1.0\">"
#define CCBI_XML_ROOT_END_PART   "</plist>"

#define CCBI_XML_START_IDENITFIER_HEAD   "<"
#define CCBI_XML_START_IDENITFIER_TAIL   ">"
#define CCBI_XML_END_IDENITFIER_HEAD     "</"
#define CCBI_XML_END_IDENITFIER_TAIL     ">"

#define CCBI_XML_NULL_VALUE_TAG_HEAD     "<"
#define CCBI_XML_NULL_VALUE_TAG_TAIL     "/>"

#define CCBI_XML_TAG_ARRAY       "array"
#define CCBI_XML_TAG_DICT        "dict"
#define CCBI_XML_TAG_KEY         "key"
#define CCBI_XML_TAG_INTEGER     "integer"
#define CCBI_XML_TAG_REAL        "real"
#define CCBI_XML_TAG_STRING      "string"

#define CCBI_XML_TAG_TRUE        "<true/>"
#define CCBI_XML_TAG_FALSE       "<false/>"

#define CCBI_XML_TAG_ARRAT_SIMPLE  "<array/>"

/**
* @brief the mapping string about sequence
*/
#define CCBI_SEQUENCE_KEY_MAIN              "sequences"
#define CCBI_SEQUENCE_KEY_MAIN_NAME         "name"
#define CCBI_SEQUENCE_KEY_DURATION_LEN      "length"
#define CCBI_SEQUENCE_KEY_SEQUENCE_ID	    "sequenceId"
#define	CCBI_SEQUENCE_KEY_CHAINEDSEQ_ID     "chainedSequenceId"
#define	CCBI_SEQUENCE_KEY_KEY_FRAMES        "keyframes"
/*callback channel*/
#define CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_NAME       "callbackChannel"
#define CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_TIME       "time"
#define CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_TYPE       "type"
/*sound channel*/
#define CCBI_SEQUENCE_SOUNDCHANNEL_KEY_NAME          "soundChannel"
#define CCBI_SEQUENCE_SOUNDCHANNEL_KEY_TIME          "time"




#define CCBI_SEQUENCE_KEY_AUTOPLAY_KEY      "autoPlay"

/**
* @brief the mapping string about nodegraph
*/
#define CCBI_NODEGRAPH_KEY_NODEGRAPH_MAIN    "nodeGraph"
#define CCBI_NODEGRAPH_KEY_BASE_CLASS        "baseClass"
#define CCBI_NODEGRAPH_KEY_JSCONTROLLER      "jsController"
#define CCBI_NODEGRAPH_KEY_CHILDREN          "children"
#define CCBI_NODEGRAPH_KEY_MEMBERVARASSIGNMENTNAME           "memberVarAssignmentName"
#define CCBI_NODEGRAPH_KEY_MEMBERVARASSIGNMENTTYPE           "memberVarAssignmentType"

/*properties*/
#define CCBI_NODEGRAPH_PROPERTIES_KEY_PROPERTIES    "properties"
#define CCBI_NODEGRAPH_PROPERTIES_KEY_NAME          "name"
#define CCBI_NODEGRAPH_PROPERTIES_KEY_TYPE          "type"
#define CCBI_NODEGRAPH_PROPERTIES_KEY_VALUE         "value"

/*animated properties*/
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_MAIN           "animatedProperties"
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_KEYFRAMES      "keyframes"
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_TYPE           "type"
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_NAME     "name"
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_TIME     "time"
#define CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_VALUE    "value"


/**
* @brief xml compound
*/
#define _XML_NULL_TAG(tag)       CCBI_XML_NULL_VALUE_TAG_HEAD##tag##CCBI_XML_NULL_VALUE_TAG_TAIL
#define XML_NULL_TAG(tag)      _XML_NULL_TAG(tag)
#define _XML_START_TAG(tag)     CCBI_XML_START_IDENITFIER_HEAD##tag##CCBI_XML_START_IDENITFIER_TAIL
#define XML_START_TAG(tag)      _XML_START_TAG(tag)
#define _XML_END_TAG(tag)       CCBI_XML_END_IDENITFIER_HEAD##tag##CCBI_XML_END_IDENITFIER_TAIL
#define XML_END_TAG(tag)        _XML_END_TAG(tag)
#define _XML_SIMPLE_ELEMENT(key, value) XML_START_TAG(key)##value##XML_END_TAG(key)
#define XML_SIMPLE_ELEMENT(key, value) _XML_SIMPLE_ELEMENT(key, value)





#endif