#ifndef WEAR_LEVELING_UTILS_H
#define WEAR_LEVELING_UTILS_H

namespace SSD_Components
{
	template <typename Block>
	unsigned int get_min_max_erase_count_difference(const Block* blocks, unsigned int block_count)
	{
		if (block_count == 0) {
			return 0;
		}

		unsigned int min_erase_count = blocks[0].Erase_count;
		unsigned int max_erase_count = min_erase_count;
		for (unsigned int i = 1; i < block_count; ++i) {
			if (blocks[i].Erase_count < min_erase_count) {
				min_erase_count = blocks[i].Erase_count;
			}
			if (blocks[i].Erase_count > max_erase_count) {
				max_erase_count = blocks[i].Erase_count;
			}
		}

		return max_erase_count - min_erase_count;
	}
}

#endif // WEAR_LEVELING_UTILS_H
