#include <iostream>
#include <fstream>
#include "../ccbanalyzer/CBIReader.h"
#include "../ccbanalyzer/ccbimapping.h"

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>

using namespace std;

void Display(vector<int>& v, const char* s);

int main(int argc, char *argv[])
{
	CCBIReader *ccbir = new CCBIReader(argv[1], argv[2]);

	/*xml head*/
	ccbir->writeXMLDeclaration();
	ccbir->writeXMLRootStartPart();
	ccbir->writeXMLDictStartTag();

	ccbir->readHeader();

	ccbir->readStringCache();

	/*write the default values*/
	ccbir->writeXMLNotes();
	ccbir->writeXMLResolutions();

	/*write the sequences into the local file*/
	ccbir->readSequences();

	/*write the nodegraph into the local file*/
	ccbir->writeXMLNodegraphHead();
	ccbir->readNodeGraph();

	/*write the xml tail*/
	ccbir->writeXMLDictEndTag();
	ccbir->writeXMLRootEndPart();

	/*
	srand(time(NULL));

	vector<int> v();

	vector<int> collection(10);
	for (int i = 0; i < 10; i++)
	{
		collection[i] = rand() % 10000;
	}

	Display(collection, "Before sorting");
	sort(collection.begin(), collection.end());
	Display(collection, "After sorting");
	*/

	printf("%s", XML_SIMPLE_ELEMENT("hello", ""));

	return 0;
}

void Display(vector<int>& v, const char* s)
{
	cout << endl << s << endl;
	copy(v.begin(), v.end(), ostream_iterator<int>(cout, "\t"));
	cout << endl;
}