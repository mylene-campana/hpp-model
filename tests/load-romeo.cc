///
/// Copyright (c) 2011 CNRS
/// Author: Florent Lamiraux
///
///
// This file is part of hpp-model
// hpp-model is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-model is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-model  If not, see
// <http://www.gnu.org/licenses/>.

#include <sstream>

#define BOOST_TEST_MODULE LOAD_ROMEO
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

#include <KineoUtility/kitParameterMap.h>

#include <kprParserXML/kprParserManager.h>

#include <KineoModel/kppComponentFactoryRegistry.h>
#include <KineoModel/kppComponent.h>
#include <KineoModel/kppModelTree.h>
#include <KineoModel/kppDeviceNode.h>
#include "KineoModel/kppLicense.h"

#include <KineoController/kppDocument.h>

#include <hpp/util/debug.hh>
#include "hpp/model/humanoid-robot.hh"
#include "hpp/model/parser.hh"
#include "hpp/model/exception.hh"

using hpp::model::Exception;

namespace hpp {
  namespace model {
    static void validateLicense()
    {
      if(!CkppLicense::initialize()) {
	throw Exception("failed to validate Kineo license.");
      }
    }
  } // namespace model
} // namespace hpp

// Define function that prints a component tree hierarchy starting
// from a root component.
void printComponent (const CkppComponentShPtr& i_component,
		     std::set<int> lastChildDepths = std::set<int> (),
		     int depth = 0);

void printComponent (const CkppComponentShPtr& i_component,
		     std::set<int> lastChildDepths,
		     int depth)
{
  std::cout << i_component->name () << std::endl;
  for (unsigned i = 0;
       i < i_component->countChildComponents ();
       ++i)
    {
      for (int j = 0; j < depth; ++j)
	if (lastChildDepths.find (j) != lastChildDepths.end ())
	  std::cout << "    ";
	else
	  std::cout << "|   ";
      std::cout << "|-> ";

      if (i == i_component->countChildComponents () - 1)
	lastChildDepths.insert (depth);

      printComponent (i_component->childComponent (i),
		      lastChildDepths,
		      depth + 1);
      lastChildDepths.erase (depth);
    }
}

BOOST_AUTO_TEST_CASE(display)
{
  hpp::model::validateLicense();

  // Create parser that will load the components.
  hpp::model::Parser extra(true);
  CkprParserManagerShPtr parser = CkprParserManager::defaultManager();

  std::string filename("./romeo-hpp.kxml");
  CkppComponentShPtr modelTreeComponent;

  CkppDocumentShPtr document =
    CkppDocument::create (CkprParserManager::defaultManager()
			  ->moduleManager ());
  CkppComponentFactoryRegistryShPtr registry
    = document->componentFactoryRegistry ();

  // Parse file and retrieve components.
  if (parser->loadComponentFromFile(filename,
				    modelTreeComponent,
				    registry,
				    CkitParameterMap::create ()) != KD_OK) {
    CkprParser::Error error = parser->lastError();
    std::string message = "failed to read " + filename + ".\n"
      + std::string(error.errorMessage());
    hppDout(error, message);
    throw Exception(message);
  }

  printComponent (modelTreeComponent);

  if (parser->writeComponentToFile("./romeo-hpp-return.kxml",
				   modelTreeComponent) != KD_OK) {
    CkprParser::Error error = parser->lastError();
    std::string message = "failed to write ./romeo-hpp-return.kxml.\n"
      + std::string(error.errorMessage());
    hppDout(error, message);
    throw Exception(message);
  }

  CkppModelTreeShPtr modelTree =
    KIT_DYNAMIC_PTR_CAST(CkppModelTree,modelTreeComponent);
  if (!modelTree)
    throw Exception("Main node is not of type CkppModelTree.");
  if(!modelTree->deviceNode())
    throw Exception("No device node in the model tree.");
  unsigned int nbDevices = modelTree->deviceNode()->countChildComponents();
  if (nbDevices != 1) {
    std::ostringstream oss;
    oss << "Expecting one device in device node, got "
	<< nbDevices << ".";
    throw Exception(oss.str());
  }
  hpp::model::HumanoidRobotShPtr humanoidRobot =
    KIT_DYNAMIC_PTR_CAST(hpp::model::HumanoidRobot,
			 modelTree->deviceNode()->childComponent(0));

  if (!humanoidRobot)
    throw Exception("Device is not of type HumanoidRobot.");

  humanoidRobot->initialize();
  output_test_stream output;
}
