#include "ModelWarperConfiguration.h"

#include <OpenSim/Common/Component.h>

#include <filesystem>

osc::mow::ModelWarperConfiguration::ModelWarperConfiguration()
{
	constructProperties();
}

osc::mow::ModelWarperConfiguration::ModelWarperConfiguration(const std::filesystem::path& filePath) :
	OpenSim::Component{filePath.string()}
{
	constructProperties();
	updateFromXMLDocument();
}

void osc::mow::ModelWarperConfiguration::constructProperties()
{
	// TODO
}
