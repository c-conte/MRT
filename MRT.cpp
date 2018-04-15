/*
Copyright (C) 2011 Georgia Institute of Technology

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A mARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* Generates square pulse current commands.
*/

#include <MRT.h>

extern "C" Plugin::Object *createRTXIPlugin(void) {
	return new IAact();
}

static DefaultGUIModel::variable_t vars[] =
{
	{ "Vin", "", DefaultGUIModel::INPUT, },
	{ "Iout", "", DefaultGUIModel::OUTPUT, },
	{ "Period (s)", "Amount of time current is injected at every step", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Delay (s)", "Time until step starts from beginning of cycle",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Current Range Start (pA)", "Starting current of the steps",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Current Range End (pA)", "Ending current of the steps", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Increment (pA)", "How many steps to take between min and max",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	{ "Cycles (#)", "How many times to repeat the protocol",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	{ "Down Time (s)", "The time between each step where IOUT is 0",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Offset (mA)", "DC offset to add", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, }, 
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

IAact::IAact(void) : DefaultGUIModel("MRT", ::vars, ::num_vars), dt(RT::System::getInstance()->getPeriod() * 1e-6), period(0.25), delay(0.0), rStart(-100), rEnd(380), step_size(20), Ncycles(1), downtime(0), offset(0.0) {
	setWhatsThis("<p><b>I-Step:</b><br>This module generates a series of currents in a designated range followed by a fixed maximum current.</p>");
	createGUI(vars, num_vars);
	update(INIT);
	refresh();
	resizeMe();
}

IAact::~IAact(void) {}

void IAact::execute(void) {
	V = input(0);  //V
	int down = 0;
	Iout = offset;
	if (cycle < Ncycles) // if the program isn't done cycling
 {
		//Do all time keeping in seconds.
		if (step < rEnd + step_size) // if the current output is less highest desired output
		{
			if (age <= downtime) // timing for downtime
				{
				  Iout = 0;
				  age += dt / 1000;
				}
			if (age >= delay + downtime && age < delay + downtime + period - EPS) 
			// if the delay is over but not period
			{
				if (step > rEnd)
					{
					  Iout = rEnd;
					  age += dt / 1000;
					}
				else 
					{
					  Iout = (Iout + rStart + step);
					  age += dt / 1000;
					}
			}
			else if (interage < downtime)
			{
			Iout = 0;
			interage += dt / 1000;
			}
			
			else
			{
				age += dt/1000;
			}
			
			if (age >= (period + downtime - EPS)) // if the time is greater than the period
 			{
				step += step_size; // the steps increase
				interage = 0;
				age = 0;
			}
			
		}
		if (step > rEnd) {
			cycle++;
			step = 0;
		}
	}

	output(0) = Iout*.5*1e-3; //mAmps. Output to amp is scaled 50mv = 100 pA
}

void IAact::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("Period (s)", period);
			setParameter("Delay (s)", delay);
			setParameter("Current Range Start (pA)", rStart);
			setParameter("Current Range End (pA)", rEnd);
			setParameter("Increment (pA)", step_size);
			setParameter("Cycles (#)", Ncycles);
			setParameter("Down Time (s)", downtime); 
			setParameter("Offset (mA)", offset);
			break;

		case MODIFY:
			period = getParameter("Period (s)").toDouble();
			delay = getParameter("Delay (s)").toDouble();
			rStart = getParameter("Current Range Start (pA)").toDouble();
			rEnd = getParameter("Current Range End (pA)").toDouble();
			step_size = getParameter("Increment (pA)").toInt();
			Ncycles = getParameter("Cycles (#)").toInt();
			downtime = getParameter("Down Time (s)").toDouble();
			offset = getParameter("Offset (mA)").toDouble();
			break;

		case PAUSE:
			output(0) = 0;
	
		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-6;
	
		default:
			break;
	}
	
	// Some Error Checking for fun
	
	if (period <= 0) {
		period = 1;
		setParameter("Period (sec)", period);
	}
	
	if (rEnd < rStart) {
		rEnd = rStart;
		setParameter("Min Amp (mA)", rStart);
		setParameter("Max Amp (mA)", rEnd);
	}
	
	if (Ncycles < 1) {
		Ncycles = 1;
		setParameter("Cycles (#)", Ncycles);
	}
	
	if (step_size < 0) {
		step_size = 0;
		setParameter("Increment (pA)", step_size);
	}
	
	if (delay <= 0 || delay > period) {
		delay = 0;
		setParameter("Delay (sec)", delay);
	}
	//Initialize counters
	age = 0;
	step = 0;
	cycle = 0;
	interage = 0;	
}
