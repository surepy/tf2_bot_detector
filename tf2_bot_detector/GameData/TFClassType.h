#pragma once

namespace tf2_bot_detector
{
	/// <summary>
	/// see: https://sm.alliedmods.net/new-api/tf2/TFClassType
	/// </summary>
	enum class TFClassType
	{
		Undefined = 0,
		Scout = 1,
		Sniper = 2,
		Soldier = 3,
		Demoman = 4,
		Medic = 5,
		Heavy = 6,
		Pyro = 7,
		Spy = 8,
		Engie = 9
	};
}
