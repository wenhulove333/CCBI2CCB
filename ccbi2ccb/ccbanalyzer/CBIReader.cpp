#include "ccbimapping.h"
#include "CBIReader.h"
#include "../util/include/ssMacro.h"
#include "../util/log/ssLog.h"

#include <algorithm>

#include <ctype.h>

#include <fstream>

using namespace std;

/*************************************************************************
Implementation of CCBIReader
*************************************************************************/
CCBIReader::CCBIReader(const char *pCCBIFile, const char *pOutCCBFile)
{
	int readbytes = 0;

	ifstream fccbi(pCCBIFile, (ios::in | ios::binary));

	/*caculate the length of ccbi file*/
	fccbi.seekg(0, ios::end);
	int len = fccbi.tellg();

	/*apply the memory buff to store the file content*/
	char *filebuff = new char[len];

	fccbi.seekg(0, ios::beg);

	while ((!fccbi.eof())
		&& (readbytes < len))
	{
		fccbi.read(filebuff, (len - readbytes));
		if (fccbi.fail())
		{			
			return;
		}

		readbytes += fccbi.gcount();
	}

	mBytes = (unsigned char*)filebuff;
	mCurrentByte = 0;
	mCurrentBit = 0;

	/*open the local file to be ready to write into the converted data*/
	outccb.open(pOutCCBFile, std::ios::out);
}

CCBIReader::~CCBIReader() {
	// Clear string cache.
	this->mStringCache.clear();
}

bool CCBIReader::readStringCache() {
	int numStrings = this->readInt(false);

	for (int i = 0; i < numStrings; i++) {
		this->mStringCache.push_back(this->readUTF8());
	}

	return true;
}

bool CCBIReader::readHeader()
{
	/* If no bytes loaded, don't crash about it. */
	if (this->mBytes == NULL) {
		return false;
	}

	/* Read magic bytes */
	int magicBytes = *((int*)(this->mBytes + this->mCurrentByte));
	this->mCurrentByte += 4;

	if ((SS_SWAP_INT32_LITTLE_TO_HOST(magicBytes) != 'CCBI')
		&& (SS_SWAP_INT32_LITTLE_TO_HOST(magicBytes) != 'ccbi')) {
		return false;
	}

	/* Read version. */
	int version = this->readInt(false);
	if (version != kCCBIVersion) {
		SSLog("WARNING! Incompatible CCBIi file version (file: %d reader: %d)", version, kCCBIVersion);
		return false;
	}

	writeXMLHeadDefault();

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "jsControlled");

	// Read JS check
	jsControlled = this->readBool();

	if (true == jsControlled)
	{
		outccb << CCBI_XML_TAG_TRUE << endl;
	}
	else
	{
		outccb << CCBI_XML_TAG_FALSE << endl;
	}

	return true;
}

unsigned char CCBIReader::readByte() {
	unsigned char byte = this->mBytes[this->mCurrentByte];
	this->mCurrentByte++;
	return byte;
}

bool CCBIReader::readBool() {
	return 0 == this->readByte() ? false : true;
}

std::string CCBIReader::readUTF8()
{
	std::string ret;

	int b0 = this->readByte();
	int b1 = this->readByte();

	int numBytes = b0 << 8 | b1;

	char* pStr = (char*)malloc(numBytes + 1);
	memcpy(pStr, mBytes + mCurrentByte, numBytes);
	pStr[numBytes] = '\0';
	ret = pStr;
	free(pStr);

	mCurrentByte += numBytes;

	return ret;
}

bool CCBIReader::getBit() {
	bool bit;
	unsigned char byte = *(this->mBytes + this->mCurrentByte);
	if (byte & (1 << this->mCurrentBit)) {
		bit = true;
	}
	else {
		bit = false;
	}

	this->mCurrentBit++;

	if (this->mCurrentBit >= 8) {
		this->mCurrentBit = 0;
		this->mCurrentByte++;
	}

	return bit;
}

void CCBIReader::alignBits() {
	if (this->mCurrentBit) {
		this->mCurrentBit = 0;
		this->mCurrentByte++;
	}
}

int CCBIReader::readInt(bool pSigned) {
	// Read encoded int
	int numBits = 0;
	while (!this->getBit()) {
		numBits++;
	}

	long long current = 0;
	for (int a = numBits - 1; a >= 0; a--) {
		if (this->getBit()) {
			current |= 1LL << a;
		}
	}
	current |= 1LL << numBits;

	int num;
	if (pSigned) {
		int s = current % 2;
		if (s) {
			num = (int)(current / 2);
		}
		else {
			num = (int)(-current / 2);
		}
	}
	else {
		num = current - 1;
	}

	this->alignBits();

	return num;
}


float CCBIReader::readFloat() {
	unsigned char type = this->readByte();

	switch (type) {
	case kCCBIFloat0:
		return 0;
	case kCCBIFloat1:
		return 1;
	case kCCBIFloatMinus1:
		return -1;
	case kCCBIFloat05:
		return 0.5f;
	case kCCBIFloatInteger:
		return (float)this->readInt(true);
	default:
	{
		/* using a memcpy since the compiler isn't
		* doing the float ptr math correctly on device.
		* TODO still applies in C++ ? */
		unsigned char* pF = (this->mBytes + this->mCurrentByte);
		float f = 0;

		// N.B - in order to avoid an unaligned memory access crash on 'memcpy()' the the (void*) casts of the source and
		// destination pointers are EXTREMELY important for the ARM compiler.
		//
		// Without a (void*) cast, the ARM compiler makes the assumption that the float* pointer is naturally aligned
		// according to it's type size (aligned along 4 byte boundaries) and thus tries to call a more optimized
		// version of memcpy() which makes this alignment assumption also. When reading back from a file of course our pointers
		// may not be aligned, hence we need to avoid the compiler making this assumption. The (void*) cast serves this purpose,
		// and causes the ARM compiler to choose the slower, more generalized (unaligned) version of memcpy()
		//
		// For more about this compiler behavior, see:
		// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka3934.html
		memcpy((void*)&f, (const void*)pF, sizeof(float));

		this->mCurrentByte += sizeof(float);
		return f;
	}
	}
}

std::string CCBIReader::readCachedString() {
	int n = this->readInt(false);
	return this->mStringCache[n];
}

void CCBIReader::readNodeGraph() {
	writeXMLDictStartTag();

	/* Read class name. */
	std::string className = this->readCachedString();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_BASE_CLASS) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << className.c_str() << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

	std::string jsControlledName;

	if (jsControlled) {
		jsControlledName = this->readCachedString();
		//outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_JSCONTROLLER) << endl;
		//outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << jsControlledName.c_str() << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
	}

	writeXMLNodegraphDefault();

	// Read assignment type and name
	int memberVarAssignmentType = this->readInt(false);
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_MEMBERVARASSIGNMENTTYPE) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << memberVarAssignmentType << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

	std::string memberVarAssignmentName;
	if (memberVarAssignmentType != kCCBITargetTypeNone) {
		memberVarAssignmentName = this->readCachedString();

		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_MEMBERVARASSIGNMENTNAME) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << memberVarAssignmentName.c_str() << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
	}

	// Read animated properties
	int numSequence = readInt(false);
	if (0 != numSequence)
	{
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_MAIN) << endl;
		writeXMLDictStartTag();
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "0") << endl;
		writeXMLDictStartTag();
	}
	for (int i = 0; i < numSequence; ++i)
	{
		int seqId = readInt(false);

		int numProps = readInt(false);

		for (int j = 0; j < numProps; ++j)
		{
			std::string animatedProp = this->readCachedString();
			const char *nameProp = animatedProp.c_str();
			outccb << XML_START_TAG(CCBI_XML_TAG_KEY) << nameProp << XML_END_TAG(CCBI_XML_TAG_KEY) << endl;

			int typeProp = readInt(false);

			int numKeyframes = readInt(false);

			/*convert to the value used for CCB xml file*/
			int convertType = CCBIMainPropTypeName::getAnimatedPropTypeValue(typeProp);

			writeXMLDictStartTag();
			outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_KEYFRAMES) << endl;
			writeXMLArrayStartTag();
			for (int k = 0; k < numKeyframes; ++k)
			{
				writeXMLDictStartTag();
				readKeyframe(typeProp, nameProp);
				writeXMLDictEndTag();
			}
			writeXMLArrayEndTag();

			outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_NAME) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << nameProp << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_TYPE) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << convertType << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

			writeXMLDictEndTag();
		}
	}

	if (0 != numSequence)
	{
		writeXMLDictEndTag();
		writeXMLDictEndTag();
	}

	// Read properties
	parseProperties();

	/* Read and add children. */
	int numChildren = this->readInt(false);
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_CHILDREN) << endl;
	if (0 != numChildren)
	{
		writeXMLArrayStartTag();
		for (int i = 0; i < numChildren; i++) {
			readNodeGraph();
		}
		writeXMLArrayEndTag();
	}
	else
	{
		outccb << CCBI_XML_TAG_ARRAT_SIMPLE << endl;
	}

	writeXMLDictEndTag();
}

void CCBIReader::readKeyframe(int type, const char *animatedpropname)
{
	float timeKeyframe = readFloat();

	int easingType = readInt(false);

	/*convert to the value used for CCB xml file*/
	int convertType = CCBIMainPropTypeName::getAnimatedPropTypeValue(type);

	float easingOpt = 0;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "easing") << endl;
	
	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_TYPE) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << easingType << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
	if (easingType == kCCBIKeyframeEasingCubicIn
		|| easingType == kCCBIKeyframeEasingCubicOut
		|| easingType == kCCBIKeyframeEasingCubicInOut
		|| easingType == kCCBIKeyframeEasingElasticIn   
		|| easingType == kCCBIKeyframeEasingElasticOut
		|| easingType == kCCBIKeyframeEasingElasticInOut)
	{
		easingOpt = readFloat();
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "Opt") << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << easingOpt << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
	}
	writeXMLDictEndTag();

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_NAME) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << animatedpropname << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_TIME) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << timeKeyframe << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_TYPE) << endl;
	outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << convertType << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_ANIMANTED_PROPERTIES_KEY_FRAME_VALUE) << endl;
	if (type == kCCBIPropTypeCheck)
	{
		bool b = readBool();
		if (true == b)
		{
			outccb << CCBI_XML_TAG_TRUE << endl;
		}
		else
		{
			outccb << CCBI_XML_TAG_FALSE << endl;
		}
	}
	else if (type == kCCBIPropTypeByte)
	{
		int i = readByte();
		outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << i << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
	}
	else if (type == kCCBIPropTypeColor3)
	{
		int r = readByte();
		int g = readByte();
		int b = readByte();

		writeXMLArrayStartTag();
		outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << r << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << g << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << b << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
		writeXMLArrayEndTag();
	}
	else if (type == kCCBIPropTypeDegrees)
	{
		float f = readFloat();
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << f << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
	}
	else if (type == kCCBIPropTypeScaleLock || type == kCCBIPropTypePosition
		|| type == kCCBIPropTypeFloatXY)
	{
		float a = readFloat();
		float b = readFloat();
		writeXMLArrayStartTag();
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << a << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << b << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
		writeXMLArrayEndTag();
	}
	else if (type == kCCBIPropTypeSpriteFrame)
	{
		std::string spriteSheet = readCachedString();
		std::string spriteFile = readCachedString();
		writeXMLArrayStartTag();
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << spriteFile << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << spriteSheet << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
		writeXMLArrayEndTag();
	}
}


bool CCBIReader::readCallbackKeyframes() {
	int numKeyframes = readInt(false);

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_NAME) << endl;
	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_KEY_FRAMES) << endl;

	if (0 == numKeyframes)
	{
		outccb << XML_NULL_TAG(CCBI_XML_TAG_ARRAY) << endl;
	}
	else
	{
		writeXMLArrayStartTag();
		for (int i = 0; i < numKeyframes; ++i) {

			float time = readFloat();
			std::string callbackName = readCachedString();

			int callbackType = readInt(false);
		}
		writeXMLArrayEndTag();
	}

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_TYPE) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "10") << endl;

	writeXMLDictEndTag();

	return true;
}

bool CCBIReader::readSoundKeyframes() {
	int numKeyframes = readInt(false);

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_SOUNDCHANNEL_KEY_NAME) << endl;
	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_KEY_FRAMES) << endl;

	if (0 == numKeyframes)
	{
		outccb << XML_NULL_TAG(CCBI_XML_TAG_ARRAY) << endl;
	}
	else
	{
		writeXMLArrayStartTag();
		for (int i = 0; i < numKeyframes; ++i) {

			float time = readFloat();
			std::string soundFile = readCachedString();
			float pitch = readFloat();
			float pan = readFloat();
			float gain = readFloat();
		}
		writeXMLArrayEndTag();
	}

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_CALLBACKCHANNEL_KEY_TYPE) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "9") << endl;

	writeXMLDictEndTag();

	return true;
}

bool CCBIReader::readSequences()
{
	/*write sequence header*/
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_MAIN) << endl;
	writeXMLArrayStartTag();

	int numSeqs = readInt(false);

	for (int i = 0; i < numSeqs; i++)
	{
		writeXMLDictStartTag();
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "autoPlay") << endl;
		outccb << CCBI_XML_TAG_TRUE << endl;

		float duration = readFloat();
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_DURATION_LEN) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << duration << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "position") << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << duration << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;

		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_MAIN_NAME) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << readCachedString().c_str() << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

		
		
		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_SEQUENCE_ID) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << readInt(false) << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

		int chainsequenceid = readInt(true);
		if (-1 != chainsequenceid)
		{
			outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_CHAINEDSEQ_ID) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << chainsequenceid << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
		}

		/*other default value setting*/
		writeXMLSequenceDefault();

		readCallbackKeyframes();
		readSoundKeyframes();

		writeXMLDictEndTag();
	}

	readInt(true);

	writeXMLArrayEndTag();
	return true;
}

std::string CCBIReader::lastPathComponent(const char* pPath) {
	std::string path(pPath);
	size_t slashPos = path.find_last_of("/");
	if (slashPos != std::string::npos) {
		return path.substr(slashPos + 1, path.length() - slashPos);
	}
	return path;
}

std::string CCBIReader::deletePathExtension(const char* pPath) {
	std::string path(pPath);
	size_t dotPos = path.find_last_of(".");
	if (dotPos != std::string::npos) {
		return path.substr(0, dotPos);
	}
	return path;
}

std::string CCBIReader::toLowerCase(const char* pString) {
	std::string copy(pString);
	std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
	return copy;
}

bool CCBIReader::endsWith(const char* pString, const char* pEnding) {
	std::string string(pString);
	std::string ending(pEnding);
	if (string.length() >= ending.length()) {
		return (string.compare(string.length() - ending.length(), ending.length(), ending) == 0);
	}
	else {
		return false;
	}
}

bool CCBIReader::isJSControlled() {
	return jsControlled;
}

void CCBIReader::parseProperties()
{
	int numRegularProps = readInt(false);
	int numExturaProps = readInt(false);
	int propertyCount = numRegularProps + numExturaProps;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_PROPERTIES) << endl;
	writeXMLArrayStartTag();

	for (int i = 0; i < propertyCount; i++) {
		bool isExtraProp = (i >= numRegularProps);
		int type = readInt(false);
		std::string propertyName = readCachedString();
		const char *propertycharsName = propertyName.c_str();
		const char *propertycharsType = CCBIMainPropTypeName::getPropTypeName(type);

		// Check if the property can be set for this platform
		bool setProp = false;

		int platform = readByte();

		writeXMLDictStartTag();

		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_NAME) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << propertycharsName << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_TYPE) << endl;
		outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << propertycharsType << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

		outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_VALUE) << endl;
		switch (type)
		{
		case kCCBIPropTypePosition:
		{
			float x = readFloat();
			float y = readFloat();
			int type = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << x << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << y << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << type << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypePoint:
		{
			float x = readFloat();
			float y = readFloat();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << x << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << y << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypePointLock:
		{
			float x = readFloat();
			float y = readFloat();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << x << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << y << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeSize: 
		{
			float width = readFloat();
			float height = readFloat();
			int type = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << width << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << height << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << type << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeScaleLock:
		{
			float x = readFloat();
			float y = readFloat();
			int type = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << x << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << y << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << CCBI_XML_TAG_FALSE << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << type << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeFloat:
		{
			float f = readFloat();

			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << f << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;

			break;
		}
		case kCCBIPropTypeFloatXY:
		{
			float x = readFloat();
			float y = readFloat();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << x << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << y << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			writeXMLArrayEndTag();

			break;
		}

		case kCCBIPropTypeDegrees:
		{
			float ret = readFloat();

			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << ret << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;

			break;
		}
		case kCCBIPropTypeFloatScale:
		{
			float f = readFloat();
			int type = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << f << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << type << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeInteger:
		{
			int i = readInt(true);

			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << i << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

			break;
		}
		case kCCBIPropTypeIntegerLabeled:
		{
			int i = readInt(true);

			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << i << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

			break;
		}
		case kCCBIPropTypeFloatVar:
		{
			float f = readFloat();
			float fVar = readFloat();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << f << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << fVar << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeCheck:
		{
			bool ret = readBool();

			if (true == ret)
			{
				outccb << CCBI_XML_TAG_TRUE << endl;
			}
			else
			{
				outccb << CCBI_XML_TAG_FALSE << endl;
			}

			break;
		}
		case kCCBIPropTypeSpriteFrame: 
		{
			std::string spritesheet = readCachedString();
			std::string spritefile = readCachedString();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << spritesheet << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << spritefile << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeAnimation:
		{
			std::string animationfile = readCachedString();
			std::string animation = readCachedString();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << animationfile << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << animation << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeTexture:
		{
			std::string spritefile = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << spritefile << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		case kCCBIPropTypeByte:
		{
			unsigned char ret = readByte();

			if (0xff == ret)
			{
				ret = 0;
			}

			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << (int)ret << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;

			break;
		}
		case kCCBIPropTypeColor3:
		{
			unsigned char red = readByte();
			unsigned char green = readByte();
			unsigned char blue = readByte();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << (int)red << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << (int)green << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << (int)blue << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeColor4FVar:
		{
			float red = readFloat();
			float green = readFloat();
			float blue = readFloat();
			float alpha = readFloat();
			float redVar = readFloat();
			float greenVar = readFloat();
			float blueVar = readFloat();
			float alphaVar = readFloat();

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << red << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << green << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << blue << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << alpha << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << redVar << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << greenVar << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << blueVar << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_REAL) << alphaVar << XML_END_TAG(CCBI_XML_TAG_REAL) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeFlip: 
		{
			bool flipX = readBool();
			bool flipY = readBool();

			writeXMLArrayStartTag();
			if (true == flipX)
			{
				outccb << CCBI_XML_TAG_TRUE << endl;
			}
			else
			{
				outccb << CCBI_XML_TAG_FALSE << endl;
			}

			if (true == flipY)
			{
				outccb << CCBI_XML_TAG_TRUE << endl;
			}
			else
			{
				outccb << CCBI_XML_TAG_FALSE << endl;
			}
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeBlendmode:
		{
			int source = readInt(false);
			int destination = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << source << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << destination << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeFntFile:
		{
			std::string fntfile = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << fntfile << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		case kCCBIPropTypeFontTTF: 
		{
			std::string fontTTF = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << fontTTF << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		case kCCBIPropTypeString: 
		{
			std::string string = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << string << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		case kCCBIPropTypeText: 
		{
			std::string text = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << text << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		case kCCBIPropTypeBlock: 
		{
			std::string selectorName = readCachedString();
			int selectorTarget = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << selectorName << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << selectorTarget << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeBlockCCControl: 
		{
			std::string selectorName = readCachedString();
			int selectorTarget = readInt(false);
			int controlEvents = readInt(false);

			writeXMLArrayStartTag();
			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << selectorName << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << selectorTarget << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			outccb << XML_START_TAG(CCBI_XML_TAG_INTEGER) << controlEvents << XML_END_TAG(CCBI_XML_TAG_INTEGER) << endl;
			writeXMLArrayEndTag();

			break;
		}
		case kCCBIPropTypeCCBIFile: 
		{
			std::string ccbFileName = readCachedString();

			outccb << XML_START_TAG(CCBI_XML_TAG_STRING) << ccbFileName << XML_END_TAG(CCBI_XML_TAG_STRING) << endl;

			break;
		}
		default:
			ASSERT_FAIL_UNEXPECTED_PROPERTYTYPE(type);
			break;
		}

		writeXMLDictEndTag();
	}



	writeXMLArrayEndTag();
}

void CCBIReader::writeXMLDeclaration()
{
	outccb << CCBI_XML_DECLARATION << endl;
}

void CCBIReader::writeXMLRootStartPart()
{
	outccb << CCBI_XML_ROOT_START_PART << endl;
}

void CCBIReader::writeXMLRootEndPart()
{
	outccb << CCBI_XML_ROOT_END_PART << endl;
}

void CCBIReader::writeXMLSequenceHead()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_SEQUENCE_KEY_MAIN) << endl;
}

void CCBIReader::writeXMLArrayStartTag()
{
	outccb << XML_START_TAG(CCBI_XML_TAG_ARRAY) << endl;
}

void CCBIReader::writeXMLArrayEndTag()
{
	outccb << XML_END_TAG(CCBI_XML_TAG_ARRAY) << endl;
}

void CCBIReader::writeXMLDictStartTag()
{
	outccb << XML_START_TAG(CCBI_XML_TAG_DICT) << endl;
}

void CCBIReader::writeXMLDictEndTag()
{
	outccb << XML_END_TAG(CCBI_XML_TAG_DICT) << endl;
}

void CCBIReader::writeXMLNotes()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "notes") << endl;
	outccb << XML_NULL_TAG(CCBI_XML_TAG_ARRAY) << endl;
}

void CCBIReader::writeXMLResolutions()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "resolutions") << endl;
	writeXMLArrayStartTag();
	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "centeredOrigin") << endl;
	outccb << CCBI_XML_TAG_FALSE << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "ext") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "iphone") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "height") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_INTEGER, "640") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "name") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "iPhone Landscape") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "scale") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_REAL, "1") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "width") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_INTEGER, "400") << endl;
	writeXMLDictEndTag();
	writeXMLArrayEndTag();
}

void CCBIReader::writeXMLNodegraphHead()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_KEY_NODEGRAPH_MAIN) << endl;
}

void CCBIReader::writeXMLHeadDefault()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "centeredOrigin") << endl;
	outccb << CCBI_XML_TAG_FALSE << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "currentResolution") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_INTEGER, "0") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "currentSequenceId") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_INTEGER, "0") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "fileType") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "CocosBuilder") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "fileVersion") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_INTEGER, "4") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "guides") << endl;
	outccb << XML_NULL_TAG(CCBI_XML_TAG_ARRAY) << endl;
}

void CCBIReader::writeXMLSequenceDefault()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "offset") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_REAL, "0.0") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "resolution") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_REAL, "30") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "scale") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_REAL, "512") << endl;
}

void CCBIReader::writeXMLNodegraphDefault()
{
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "customClass") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "") << endl;

	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "displayName") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "ccbi2ccbdefault") << endl;
}

void CCBIReader::writeXMLNodegraphPropDefault()
{
	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_NAME) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "touchEnabled") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "platform") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "iOS") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_TYPE) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "Check") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_VALUE) << endl;
	outccb << CCBI_XML_TAG_TRUE << endl;
	writeXMLDictEndTag();

	writeXMLDictStartTag();
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_NAME) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "mouseEnabled") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, "platform") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "Mac") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_TYPE) << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_STRING, "Check") << endl;
	outccb << XML_SIMPLE_ELEMENT(CCBI_XML_TAG_KEY, CCBI_NODEGRAPH_PROPERTIES_KEY_VALUE) << endl;
	outccb << CCBI_XML_TAG_TRUE << endl;
	writeXMLDictEndTag();
}

const char *CCBIMainPropTypeName::typeName[kCCBIPropTypeMAX + 1] =
{
	"Position",
	"Size",
	"Point",
	"PointLock",
	"ScaleLock",
	"Degrees",
	"Integer",
	"Float",
	"FloatVar",
	"Check",
	"SpriteFrame",
	"Texture",
	"Byte",
	"Color3",
	"Color4FVar",
	"Flip",
	"Blendmode",
	"FntFile",
	"Text",
	"FontTTF",
	"IntegerLabeled",
	"Block",
	"Animation",
	"CCBFile",
	"String",
	"BlockCCControl",
	"FloatScale",
	"FloatXY",
	"null"
};

const int CCBIMainPropTypeName::animatedproptypevalue[kCCBIPropTypeMAX + 1] =
{
	3,
	-1,
	-1,
	-1,
	4,
	2,
	-1,
	-1,
	-1,
	1,
	7,
	-1,
	5,
	6,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

CCBIMainPropTypeName::CCBIMainPropTypeName()
{

}

const char* CCBIMainPropTypeName::getPropTypeName(int typevalue)
{
	return typeName[typevalue];
}

const int CCBIMainPropTypeName::getAnimatedPropTypeValue(int typevalue)
{
	return animatedproptypevalue[typevalue];
}

