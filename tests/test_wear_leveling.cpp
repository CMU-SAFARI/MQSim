#include "Wear_Leveling_Utils.h"

#include <cstdio>
#include <cstdlib>

struct TestBlock
{
	unsigned int Erase_count;
};

static void check_difference(const char* name, const TestBlock* blocks,
		unsigned int count, unsigned int expected)
{
	const unsigned int actual = SSD_Components::get_min_max_erase_count_difference(blocks, count);
	if (actual != expected) {
		std::fprintf(stderr, "%s: expected %u, got %u\n", name, expected, actual);
		std::exit(EXIT_FAILURE);
	}
}

int main()
{
	const TestBlock max_after_min[] = {{12}, {14}, {305}, {20}};
	const TestBlock max_before_min[] = {{300}, {20}, {15}, {17}};
	const TestBlock equal[] = {{42}, {42}, {42}};

	check_difference("max block after min block", max_after_min, 4, 293);
	check_difference("max block before min block", max_before_min, 4, 285);
	check_difference("equal erase counts", equal, 3, 0);
	check_difference("empty plane", nullptr, 0, 0);
	std::puts("wear-leveling threshold tests passed");
	return EXIT_SUCCESS;
}
