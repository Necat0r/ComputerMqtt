
#pragma once

#include <string>
#include <vector>

// Home assistant device discovery types

namespace ha
{

// https://www.home-assistant.io/integrations/mqtt

// Not a complete listing, primarily only what's necessary for this app
struct Component
{
	std::string name;
	std::string uniqueId;

	std::string deviceClass;

	struct Availability
	{
		std::string topic;

		std::string payloadAvailable = "true";
		std::string payloadNotAvailable = "false";
	};
	Availability availability;	// This is really an array in the message, but we don't really need it.

	std::string stateTopic;
	std::string commandTopic;

	std::string icon;
};

struct Switch : Component
{
	static const char* Platform;

	std::string payloadOn = "true";
	std::string payloadOff = "false";
};

struct Number : Component
{
	static const char* Platform;

	float min = 1.f;
	float max = 100.f;

	enum class Mode
	{
		Auto,	// Omitted
		Box,
		Slider,
	};
	Mode mode;

	std::string unitOfMeasurement;	// E.g. "%". device-class can be used for more standardized units like "temperature"
};

struct Discovery
{
	struct Device
	{
		std::string name;
		std::string identifier;
	};
	Device device;

	struct Origin
	{
		std::string name;
		std::string softwareVersion;
		std::string supportUrl;
	};
	Origin origin;

	std::vector<Switch> switches;
	std::vector<Number> numbers;
};

std::string toJson(Discovery discovery);

}
