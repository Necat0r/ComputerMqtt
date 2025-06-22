#include "HomeAssistant.h"

#include <nlohmann/json.hpp>

namespace ha
{

const char* Switch::Platform = "switch";
const char* Number::Platform = "number";

std::string ha::toJson(Discovery discovery)
{
	using json = nlohmann::json;

	json payload = {
		{ "device", {
			{ "name", discovery.device.name },
			{ "identifiers", json::array({ discovery.device.identifier })},

			{ "mf", "Bla electronics" },
			{ "mdl", "xya" },
			{ "sw", "1.0" },
			{ "sn",  "ea334450945afc"},
			{"hw",  "1.0rev2"}

		}},
		{ "origin", {
			{ "name", discovery.origin.name },
			{ "sw_version", discovery.origin.softwareVersion },
			{ "support_url", discovery.origin.supportUrl }
		}},
	};

	json& components = payload["components"];

	auto convertCommon = [](const Component& c) -> json {
		json j = {
			{ "name", c.name },
			{ "unique_id", c.uniqueId },
			{ "availability", json::array({{
				{ "payload_available", c.availability.payloadAvailable },
				{ "payload_not_available", c.availability.payloadNotAvailable },
				{ "topic", c.availability.topic }
			}})},
			{ "state_topic", c.stateTopic },
			{ "command_topic", c.commandTopic },
		};

		if (!c.deviceClass.empty())
			j["device_class"] = c.deviceClass;

		if (!c.icon.empty())
			j["icon"] = c.icon;

		return j;
	};

	// Switches
	for (const Switch& sw : discovery.switches)
	{
		json component = convertCommon(sw);

		component["platform"] = Switch::Platform;

		// Set the switch unique fields
		component["payload_on"] = sw.payloadOn;
		component["payload_off"] = sw.payloadOff;

		components[sw.name] = std::move(component);
	}

	// Numbers
	for (const Number& number : discovery.numbers)
	{
		json component = convertCommon(number);

		component["platform"] = Number::Platform;

		component["min"] = number.min;
		component["max"] = number.max;

		switch (number.mode)
		{
		case Number::Mode::Auto:
			// Omit it
			break;

		case Number::Mode::Box:
			component["mode"] = "box";
			break;

		case Number::Mode::Slider:
			component["mode"] = "slider";
			break;

		default:
			assert(false && "Unhandled enum");
		}

		component["unit_of_measurement"] = number.unitOfMeasurement;

		components[number.name] = std::move(component);
	}

	return payload.dump();
}

}