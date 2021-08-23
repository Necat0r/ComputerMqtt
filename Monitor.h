#pragma once

#include <functional>

class Monitor
{
public:
	using PowerCallback = std::function<void(bool)>;

	Monitor(PowerCallback&& callback);
	~Monitor();

	void setPower(bool value);

private:
	void changeStatus(bool value);

	PowerCallback m_powerCallback;
	bool m_monitorOn;
};