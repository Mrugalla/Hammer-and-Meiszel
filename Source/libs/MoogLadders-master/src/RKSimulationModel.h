/*
Copyright (c) 2015, Miller Puckette. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include "LadderFilterBase.h"

/*
Imitates a Moog resonant filter by Runge-Kutte numerical integration of
a differential equation approximately describing the dynamics of the circuit.
 
Useful references:

	* Tim Stilson
	"Analyzing the Moog VCF with Considerations for Digital Implementation"
		Sections 1 and 2 are a reasonably good introduction but the 
		model they use is highly idealized.

	* Timothy E. Stinchcombe
	"Analysis of the Moog Transistor Ladder and Derivative Filters"
		Long, but a very thorough description of how the filter works including
		its nonlinearities

	* Antti Huovilainen
	"Non-linear digital implementation of the moog ladder filter"
		Comes close to giving a differential equation for a reasonably realistic
		model of the filter

The differential equations are:

	y1' = k * (S(x - r * y4) - S(y1))
	y2' = k * (S(y1) - S(y2))
	y3' = k * (S(y2) - S(y3))
	y4' = k * (S(y3) - S(y4))

where k controls the cutoff frequency, r is feedback (<= 4 for stability), and S(x) is a saturation function.
*/

namespace moog
{
	struct RKSimulationMoog :
		public LadderFilterBase
	{
		RKSimulationMoog() :
			saturation(3.),
			saturationInv(1. / saturation),
			oversampleFactor(1),
			stepSize(1. / (oversampleFactor * sampleRate))
		{
			memset(state, 0, sizeof(state));
			setResonance(1.0f);
		}

		double operator()(double sample) noexcept override
		{
			for (int j = 0; j < oversampleFactor; ++j)
				rungekutteSolver(sample);

			return state[3];
		}

		void setResonance(double r) noexcept override
		{
			// 0 to 10
			resonance = r;
		}

		void setCutoff(double c) noexcept override
		{
			cutoff = MOOG_TAU * c;
		}

	private:

		void calculateDerivatives(double input, double* _dstate, double* _state) noexcept
		{
			auto satstate0 = clip(_state[0], saturation, saturationInv);
			auto satstate1 = clip(_state[1], saturation, saturationInv);
			auto satstate2 = clip(_state[2], saturation, saturationInv);

			_dstate[0] = cutoff * (clip(input - resonance * _state[3], saturation, saturationInv) - satstate0);
			_dstate[1] = cutoff * (satstate0 - satstate1);
			_dstate[2] = cutoff * (satstate1 - satstate2);
			_dstate[3] = cutoff * (satstate2 - clip(_state[3], saturation, saturationInv));
		}

		void rungekutteSolver(double input) noexcept
		{
			int i;
			double deriv1[4], deriv2[4], deriv3[4], deriv4[4], tempState[4];

			calculateDerivatives(input, deriv1, state);

			for (i = 0; i < 4; ++i)
				tempState[i] = state[i] + .5 * stepSize * deriv1[i];

			calculateDerivatives(input, deriv2, tempState);

			for (i = 0; i < 4; ++i)
				tempState[i] = state[i] + .5 * stepSize * deriv2[i];

			calculateDerivatives(input, deriv3, tempState);

			for (i = 0; i < 4; ++i)
				tempState[i] = state[i] + stepSize * deriv3[i];

			calculateDerivatives(input, deriv4, tempState);

			for (i = 0; i < 4; ++i)
				state[i] += (1. / 6.) * stepSize * (deriv1[i] + 2. * deriv2[i] + 2. * deriv3[i] + deriv4[i]);
		}

		double state[4];
		double saturation, saturationInv;
		int oversampleFactor;
		double stepSize;
	};
}

