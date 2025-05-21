#include<string>
#include <vector>
namespace utils {
    void spawn_console();
    std::vector<std::string> split(const std::string& str, const std::string& delim);
    bool isValidName(const char* name);
	const char* pickMoreReadable(const char* derefName, const char* thisName);
	void pause();
	void nop();
	void log(std::string data);
	namespace asmb {
		extern std::vector<uint8_t> jmp;
		extern std::vector<uint8_t> nop;
		extern std::vector<uint8_t> mov;
		extern std::vector<uint8_t> ret;
		extern std::vector<uint8_t> call;
		extern std::vector<uint8_t> saveRegisters;
		extern std::vector<uint8_t> restoreRegisters;
	}
    void createHook(long long hookLocation, const std::vector<uint8_t>& oldCode,
        const std::vector<uint8_t>& befMyFunc, void* myFunc, const std::vector<uint8_t>& aftMyFunc);
	std::vector<uintptr_t> scanMemoryForValue(uint64_t targetValue);
}