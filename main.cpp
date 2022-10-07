/**
  *
  * Copyright (c) 2022 SMMB-DEV
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  * 
*/



#include <iostream>
#include <string>
#include <sstream>
#include <functional>
#include <iomanip>

#define NOMINMAX
#include <Windows.h>



namespace Console
{
	static COORD CURRENT_COORD;
	static HANDLE CONSOLE_HANDLE = GetStdHandle(STD_OUTPUT_HANDLE);

	void Init()
	{
		//DWORD mode;
		//GetConsoleMode(CONSOLE_HANDLE, &mode);
		//mode |= DISABLE_NEWLINE_AUTO_RETURN | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		//SetConsoleMode(CONSOLE_HANDLE, mode);

		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(CONSOLE_HANDLE, &info);
		info.dwSize.X = 140;
		SetConsoleScreenBufferSize(CONSOLE_HANDLE, info.dwSize);
		
		info.srWindow.Right = info.dwSize.X - 1;
		SetConsoleWindowInfo(CONSOLE_HANDLE, TRUE, &info.srWindow);
	}

	void SaveCursorPosition()
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(CONSOLE_HANDLE, &info);		//todo: check return value
		CURRENT_COORD = info.dwCursorPosition;
	}

	void RestoreCursorPosition(size_t erase)
	{
		SetConsoleCursorPosition(CONSOLE_HANDLE, CURRENT_COORD);
		printf("%*c", erase, ' ');
		SetConsoleCursorPosition(CONSOLE_HANDLE, CURRENT_COORD);
	}

	void Color(WORD color)
	{
		SetConsoleTextAttribute(CONSOLE_HANDLE, color);
	}
}



void GetInput(const char* msg, const std::function< bool(std::string&) >&& input_ok)
{
	if (msg)	//necessary?
		std::cout << msg;

	Console::SaveCursorPosition();

	while (true)
	{
		std::string input;

		if (!std::getline(std::cin, input).good())
		{
			std::cin.clear();
			continue;
		}

		if (!input_ok || input_ok(input))
			break;

		Console::RestoreCursorPosition(input.length());
	}
}

template<typename T>
void GetNumber(const char* const msg, T& t, const T min = std::numeric_limits<T>::min(), const T max = std::numeric_limits<T>::max(), bool optional = false)
{
	static_assert(std::is_arithmetic_v<T> && sizeof(T) >= 2, "T can only be an integer or floating point type!");

	if (msg)
		std::cout << msg;

	Console::SaveCursorPosition();

	while (true)
	{
		std::string input;

		if (!std::getline(std::cin, input).good())
		{
			std::cin.clear();
			continue;
		}

		if (optional && input.empty())
			break;

		std::istringstream ss(input);	//copies the string!!

		ss >> t;
		if (ss.rdstate() == std::istringstream::eofbit && t >= min && t <= max) // && (isValid ? isValid(t) : true))
			break;

		Console::RestoreCursorPosition(input.length());
	}
}



namespace Inputs
{
	// YES, I'M USING GLOBAL VARIABLES!!!
	// To avoid passing them around to multiple functions.

	static double Vcc, Vout;
	static uint32_t Min = 1, Max = std::numeric_limits<uint32_t>::max();
}



namespace R
{
	class E
	{
	protected:
		using value_type = const uint16_t;

		//https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
		class iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using difference_type = int16_t;
			using value_type = E::value_type;
			using pointer = value_type*;
			using reference = value_type&;
			
			iterator(pointer ptr, difference_type step) : ptr(ptr), step(step) {}

			friend bool operator== (const iterator& a, const iterator& b) { return a.ptr == b.ptr; } //&& a.step == b.step; };
			friend bool operator!= (const iterator& a, const iterator& b) { return a.ptr != b.ptr; } //&& a.step == b.step; };

			reference operator*() const { return *ptr; }
			pointer operator->() { return ptr; }

			iterator& operator++() { ptr += step; return *this; }
			iterator& operator--() { ptr -= step; return *this; }

			iterator operator++(int) { iterator temp = *this; ++(*this); return temp; }
			iterator operator--(int) { iterator temp = *this; --(*this); return temp; }

			iterator& operator+=(difference_type d) { ptr += d * step; return *this; }
			iterator& operator-=(difference_type d) { ptr -= d * step; return *this; }

			iterator operator+(difference_type d) { return iterator(ptr + d * step, step); }
			iterator operator-(difference_type d) { return iterator(ptr - d * step, step); }

			//friend difference_type operator-(const iterator& a, const iterator& b) { return (a.ptr - b.ptr) / a.step; }

			reference operator[](difference_type d) { return ptr[d * step]; }

			friend bool operator< (const iterator& a, const iterator& b) { return a.ptr < b.ptr; }
			friend bool operator> (const iterator& a, const iterator& b) { return a.ptr > b.ptr; }
			friend bool operator<= (const iterator& a, const iterator& b) { return a.ptr <= b.ptr; }
			friend bool operator>= (const iterator& a, const iterator& b) { return a.ptr >= b.ptr; }

		private:
			pointer ptr;
			difference_type step;
		};

		int8_t MakeInRange(double& R) const noexcept(false)
		{
			if (R <= 0)
				throw std::out_of_range("");


			//make 100 <= R < 1000
			int8_t correction = 0;

			while (R < first())
			{
				R *= 10;
				correction--;
			}

			while (R >= first() * 10)
			{
				R /= 10;
				correction++;
			}

			return correction;
		}

	public:
		virtual iterator begin() const = 0;
		virtual iterator end() const = 0;

		virtual std::streamsize SignificantDigits() const = 0;

		virtual value_type first() const = 0;
		virtual value_type last() const = 0;

		virtual double errL() const = 0;
		virtual double errH() const = 0;

		virtual struct Result { int8_t LC, HC; uint16_t L, H; } find(double R) const noexcept(false) = 0;
	};

	class E24Group : public E
	{
		static constexpr uint16_t E24V[24] =	//Every 2nd number for E12, every 4th number for E6 and every 8th number for E3
		{
			100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300, 330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910
		};

		const E::iterator::difference_type step;

	protected:
		E24Group(E::iterator::difference_type step) : step(step) {}

	public:
		virtual E::iterator begin() const final override { return E::iterator(&E24V[0], step); }
		virtual E::iterator end() const final override { return E::iterator(&E24V[24], step); }

		virtual std::streamsize SignificantDigits() const final override { return 2; }

		virtual value_type first() const final override { return E24V[0]; }
		virtual value_type last() const final override { return E24V[24 - step]; }

		virtual Result find(double R) const noexcept(false) final override
		{
			int8_t correction = MakeInRange(R);	//100 - 1000

			for (uint8_t i = 0; i < _countof(E24V); i += step)
			{
				if (E24V[i] > R)	//for the first time
					return Result{ correction, correction, E24V[i - step], E24V[i] };	//i will never be 0
				else if (E24V[i] == R)
					return Result{ correction, correction, E24V[i], E24V[i] };
			}

			// R > last
			return Result{ correction, ++correction, last(), first() };
		}
	};

	class E192Group : public E
	{
		static constexpr uint16_t E192V[192] =	//Every second number for E96 and every 4th number for E48
		{
			100, 101, 102, 104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129, 130, 132,
			133, 135, 137, 138, 140, 142, 143, 145, 147, 149, 150, 152, 154, 156, 158, 160, 162, 164, 165, 167, 169, 172, 174, 176,
			178, 180, 182, 184, 187, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 221, 223, 226, 229, 232, 234,
			237, 240, 243, 246, 249, 252, 255, 258, 261, 264, 267, 271, 274, 277, 280, 284, 287, 291, 294, 298, 301, 305, 309, 312,
			316, 320, 324, 328, 332, 336, 340, 344, 348, 352, 357, 361, 365, 370, 374, 379, 383, 388, 392, 397, 402, 407, 412, 417,
			422, 427, 432, 437, 442, 448, 453, 459, 464, 470, 475, 481, 487, 493, 499, 505, 511, 517, 523, 530, 536, 542, 549, 556,
			562, 569, 576, 583, 590, 597, 604, 612, 619, 626, 634, 642, 649, 657, 665, 673, 681, 690, 698, 706, 715, 723, 732, 741,
			750, 759, 768, 777, 787, 796, 806, 816, 825, 835, 845, 856, 866, 876, 887, 898, 909, 920, 931, 942, 953, 965, 976, 988
		};

		const E::iterator::difference_type step;

	protected:
		E192Group(E::iterator::difference_type step) : step(step) {}

	public:
		virtual E::iterator begin() const final override { return E::iterator(&E192V[0], step); }
		virtual E::iterator end() const final override { return E::iterator(&E192V[192], step); }

		virtual std::streamsize SignificantDigits() const final override { return 3; }

		virtual value_type first() const final override { return E192V[0]; }
		virtual value_type last() const final override { return E192V[192 - step]; }

		virtual Result find(double R) const noexcept(false) final override
		{
			int8_t correction = MakeInRange(R);	//100 - 1000

			for (uint8_t i = 0; i < _countof(E192V); i += step)
			{
				if (E192V[i] > R)	//for the first time
					return Result{ correction, correction, E192V[i - step], E192V[i] };	//i will never be 0
				else if (E192V[i] == R)
					return Result{ correction, correction, E192V[i], E192V[i] };
			}

			// R > last
			return Result{ correction, ++correction, last(), first() };
		}
	};

	class E3 : public E24Group
	{
	public:
		E3() : E24Group(8) {}

		virtual double errL() const final override { return 0.80; }
		virtual double errH() const final override { return 1.20; }
	};

	class E6 : public E24Group
	{
	public:
		E6() : E24Group(4) {}

		virtual double errL() const final override { return 0.80; }
		virtual double errH() const final override { return 1.20; }
	};

	class E12 : public E24Group
	{
	public:
		E12() : E24Group(2) {}

		virtual double errL() const final override { return 0.90; }
		virtual double errH() const final override { return 1.10; }
	};

	class E24 : public E24Group
	{
	public:
		E24() : E24Group(1) {}

		virtual double errL() const final override { return 0.95; }
		virtual double errH() const final override { return 1.05; }
	};

	class E48 : public E192Group
	{
	public:
		E48() : E192Group(4) {}

		virtual double errL() const final override { return 0.98; }
		virtual double errH() const final override { return 1.02; }
	};

	class E96 : public E192Group
	{
	public:
		E96() : E192Group(2) {}

		virtual double errL() const final override { return 0.99; }
		virtual double errH() const final override { return 1.01; }
	};

	class E192 : public E192Group
	{
	public:
		E192() : E192Group(1) {}

		virtual double errL() const final override { return 0.995; }
		virtual double errH() const final override { return 1.005; }
	};

	

	bool ExpToUnit(const uint16_t R1, const uint16_t R2, int8_t& R1C, int8_t& R2C, char& R1U, char& R2U,
		std::streamsize& R1Precision, std::streamsize& R2Precision, const E& e)
	{
		// make R1C , R2C >= 0 (by default the smaller resistance is >= 1)
		const int8_t add = std::max(0 - R1C, 0 - R2C);

		R1C += add;
		R2C += add;



		// Min/Max
		double biggerThanMin;
		while ((biggerThanMin = R1 * std::pow(10, R1C) + R2 * std::pow(10, R2C)) < Inputs::Min)
		{
			R1C++;
			R2C++;
		}

		const auto diff = std::abs(R1C - R2C) % 3;
		double smallerThanMax;
		while ((smallerThanMax = R1 * std::pow(10, R1C) + R2 * std::pow(10, R2C)) > Inputs::Max && R1C > -2 + diff && R2C > -2 + diff)
		{
			R1C--;
			R2C--;
		}

		if (smallerThanMax < biggerThanMin)
			return false;



		//Units
		static constexpr char units[] = { ' ', 'K', 'M', 'G' };
		const std::streamsize significantDigits = e.SignificantDigits();

		uint8_t unitIndex = R1C / 3 + (R1C > 0);
		R1C -= unitIndex * 3;
		R1U = units[unitIndex];
		R1Precision = std::max(-R1C - (3 - significantDigits), 0ll);

		unitIndex = R2C / 3 + (R2C > 0);
		R2C -= unitIndex * 3;
		R2U = units[unitIndex];
		R2Precision = std::max(-R2C - (3 - significantDigits), 0ll);

		return true;
	}

	void SetErrorColor(double error)
	{
		error = std::abs(error);

		if (error < 0.01)
			Console::Color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		else if (error < 1)
			Console::Color(FOREGROUND_GREEN);
		else if (error > 10)
			Console::Color(FOREGROUND_RED);
		else if (error > 5)
			Console::Color(FOREGROUND_RED | FOREGROUND_INTENSITY);
		else if (error > 2)
			Console::Color(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		else
			Console::Color(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
	}

	void ShowcaseErrorColors()
	{
		std::cout << "\nError Colors: ";

		SetErrorColor(10.1);
		std::cout << "> 10%    ";

		SetErrorColor(5.1);
		std::cout << "> 5%    ";

		SetErrorColor(2.1);
		std::cout << "> 2%    ";

		SetErrorColor(1.5);
		std::cout << "1-2%    ";

		SetErrorColor(0.9);
		std::cout << "< 1%    ";

		SetErrorColor(0.009);
		std::cout << "< 0.01%" << std::endl;
	}

	void Print(uint16_t R1, uint16_t R2, int8_t R1C, int8_t R2C, const double ratio, const E& e, const bool vcc_vout)
	{
		char R1U, R2U;
		std::streamsize R1Precision, R2Precision;

		const double _R1 = R1 * std::pow(10, R1C), _R2 = R2 * std::pow(10, R2C);

		if (ExpToUnit(R1, R2, R1C, R2C, R1U, R2U, R1Precision, R2Precision, e))
		{
			double	foundRatio	= vcc_vout ? (_R1 + _R2) / _R2 : _R2 / (_R1 + _R2),
					error		= foundRatio / ratio - 1,
					voltageDiff	= (vcc_vout ? Inputs::Vcc : Inputs::Vout) * error;

			error *= 100;

			SetErrorColor(error);

			//assuming error doesn't need more than 4 significant digits (~1x.xx%) and setting field width to 6 (including the point '.' and sign '+/-')

			//Nominal
			std::cout << std::noshowpos << std::fixed <<
				"R1: " << std::setprecision(R1Precision) << R1 * std::pow(10, R1C) << R1U <<
				"\tR2: " << std::setprecision(R2Precision) << R2 * std::pow(10, R2C) << R2U <<
				(vcc_vout ? " \tVcc" : " \tVout") << " Error:" <<
				"\tNom: " << std::showpos << std::setprecision(2) << std::setw(6) << error << "% (" <<
				voltageDiff << " V)";

			//Low
			const double _R1L = _R1 * e.errH();
			const double _R2L = _R2 * e.errL();

			foundRatio = vcc_vout ? (_R1L + _R2L) / _R2L : _R2L / (_R1L + _R2L);
			error = foundRatio / ratio - 1;
			voltageDiff = (vcc_vout ? Inputs::Vcc : Inputs::Vout) * error;
			error *= 100;

			std::cout << "\t\tMin: " << std::setw(6) << error << "% (" << voltageDiff << " V)";


			//High
			const double _R1H = _R1 * e.errL();
			const double _R2H = _R2 * e.errH();

			foundRatio = vcc_vout ? (_R1H + _R2H) / _R2H : _R2H / (_R1H + _R2H);
			error = foundRatio / ratio - 1;
			voltageDiff = (vcc_vout ? Inputs::Vcc : Inputs::Vout) * error;
			error *= 100;

			std::cout << "\t\tMax: " << std::setw(6) << error << "% (" << voltageDiff << " V)";



			std::cout << std::endl;
		}
	}
}





int main()
{
	Console::Init();



	//Select resistors' tolerance
	R::E* toleranceSelection = nullptr;
	GetInput("Enter desired series:\n1) E3 (> 20%)\n2) E6(20%)\n3) E12(10%)\n4) E24(5% & 1%)\n5) E48(2%)\n6) E96(1%)\n7) E192(<= 0.5%)\n",
		[&toleranceSelection](std::string& input) -> bool
		{
			if (input.length() == 1)
			{
				switch (input[0])
				{
					case '1': toleranceSelection = new R::E3; break;
					case '2': toleranceSelection = new R::E6; break;
					case '3': toleranceSelection = new R::E12; break;
					case '4': toleranceSelection = new R::E24; break;
					case '5': toleranceSelection = new R::E48; break;
					case '6': toleranceSelection = new R::E96; break;
					case '7': toleranceSelection = new R::E192; break;

					default:
						return false;
				}

				static constexpr const char* series[] = { "E3 (> 20%)", "E6(20%)", "E12(10%)", "E24(5% & 1%)", "E48(2%)", "E96(1%)", "E192(<= 0.5%)" };

				std::cout << "Selected series: " << series[input[0] - '1'] << "\n" << std::endl;
				return true;
			}

			return false;
		});

	

	//Vcc and Vout
	GetNumber<double>("Enter Vcc (positive): ", Inputs::Vcc, std::numeric_limits<double>::epsilon());
	GetNumber<double>("Enter Vout (smaller than Vcc): ", Inputs::Vout, std::numeric_limits<double>::epsilon(), Inputs::Vcc);
	


	// Vcc/Vout
	bool vcc_vout;	//0: Vcc, 1: Vout
	GetInput("\nWhich one should be kept constant: Vcc(C) or Vout(O)? ", [&vcc_vout](std::string& input) -> bool
		{
			if (input.length() == 1)
			{
				char _entered = input[0];

				if (_entered == 'c' || _entered == 'C')
				{
					vcc_vout = false;
					return true;
				}
				else if (_entered == 'o' || _entered == 'O')
				{
					vcc_vout = true;
					return true;
				}
			}

			return false;
		});

	std::cout << "Selection: " << (vcc_vout ? "Vout" : "Vcc") << "\n" << std::endl;



	// High/Low
	bool higher = false, lower = false;

	std::cout << "Do you want the other voltage (" << (vcc_vout ? "Vcc" : "Vout") <<
		") to be calculated to be lower(L) or higher(H) than the entered number(" << (vcc_vout ? Inputs::Vcc : Inputs::Vout) << ")? (Optional) ";
	GetInput(nullptr, [&higher, &lower](std::string& input) -> bool
		{
			if (input.length() == 1)
			{
				char _entered = input[0];

				if (_entered == 'l' || _entered == 'L')
					lower = true;

				if (_entered == 'h' || _entered == 'H')
					higher = true;
			}
			else if (input.empty())
				lower = higher = true;

			return lower || higher;
		});

	std::cout << "Selection: " << (higher ? (lower ? "Higher/Lower" : "Higher") : "Lower") << "\n" << std::endl;



	// Minimum/Maximum
	GetNumber<uint32_t>("Enter minimum total resistance (1(default)-1,000,000,000) - Optional: ", Inputs::Min, 1, 1'000'000'000, true);
	GetNumber<uint32_t>("Enter maximum total resistance (min-1,000,000,000) - Optional: ", Inputs::Max, Inputs::Min, 1'000'000'000, true);



	//Error Colors
	R::ShowcaseErrorColors();

	R::SetErrorColor(5.1);
	std::cout << "\nNote:"
		"\n* Min and max errors are calculated for when R1 and R2 have maximium opposite errors (e.g. +5% and -5%)! *"
		"\n* Error color is set based on nominal error. *"
		"\n* Min/Max error might have opposite sign of nominal error even if you previously chose only negative/positive error;"
		" in other words, the selection is made based on nominal error and min/max error is not considered! *\n" << std::endl;



	//Calculate
	double ratio = vcc_vout ? Inputs::Vcc / Inputs::Vout : Inputs::Vout / Inputs::Vcc;

	if (vcc_vout)
	{
		//Vout constant

		for (const auto& R2 : *toleranceSelection)
		{
			double R1_true = R2 * (ratio - 1);
			auto R1_approx = toleranceSelection->find(R1_true);

			if (lower)
				R::Print(R1_approx.L, R2, R1_approx.LC, 0, ratio, *toleranceSelection, vcc_vout);

			if (higher)
				R::Print(R1_approx.H, R2, R1_approx.HC, 0, ratio, *toleranceSelection, vcc_vout);
		}
	}
	else
	{
		//Vcc constant

		for (const auto& R1 : *toleranceSelection)
		{
			double R2_true = R1 * ratio / (1 - ratio);
			auto R2_approx = toleranceSelection->find(R2_true);

			if (lower)
				R::Print(R1, R2_approx.L, 0, R2_approx.LC, ratio, *toleranceSelection, vcc_vout);

			if (higher)
				R::Print(R1, R2_approx.H, 0, R2_approx.HC, ratio, *toleranceSelection, vcc_vout);
		}
	}

	delete toleranceSelection;



	Console::Color(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
	system("pause");

	return 0;
}

