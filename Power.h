#pragma once
#include <functional>

void initPowerNotifications(std::function<void(bool)>&& callback);
void standby();