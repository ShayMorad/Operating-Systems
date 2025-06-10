#include "PhysicalMemory.h"
#define ERROR (-1)

word_t translate_address (uint64_t v_address, int levels);
word_t locate_frame_for_insertion (word_t input_page, word_t
previous_frames[TABLES_DEPTH]);

bool check_prior_frame (word_t current_frame,
                        const word_t previous_frames[TABLES_DEPTH])
{
  for (int index = 0; index < TABLES_DEPTH; ++index)
  {
    if (current_frame == previous_frames[index])
    {
      return true;
    }
  }
  return false;
}

void clear_frame (word_t frame)
{
  for (int offset = 0; offset < PAGE_SIZE; ++offset)
  {
    PMwrite (frame * PAGE_SIZE + offset, 0);
  }
}

void delete_connection (word_t source_frame, word_t end_frame, int level)
{
  if (level >= TABLES_DEPTH)
  {
    return;
  }

  word_t value;
  for (int idx = 0; idx < PAGE_SIZE; ++idx)
  {
    PMread (source_frame * PAGE_SIZE + idx, &value);

    if (value == end_frame)
    {
      PMwrite (source_frame * PAGE_SIZE + idx, 0);
    }
    else
    {
      delete_connection (value, end_frame, level + 1);
    }
  }
}

bool check_if_last_frame (word_t source_frame, word_t end_frame, int level)
{
  if (level >= TABLES_DEPTH)
  {
    return false;
  }

  word_t val;

  for (int index = 0; index < PAGE_SIZE; ++index)
  {
    PMread (source_frame * PAGE_SIZE + index, &val);

    if (val == 0)
    {
      continue;
    }
    else if (val == end_frame)
    {
      return (level == TABLES_DEPTH - 1);
    }
    else if (check_if_last_frame (val, end_frame, level + 1))
    {
      return true;
    }
  }

  return false;
}

word_t retrieve_frame (word_t page_num)
{
  word_t frame_addr = 0;

  for (int level = 0; level < TABLES_DEPTH; ++level)
  {
    uint64_t shifted_page =
        page_num >> (OFFSET_WIDTH * (TABLES_DEPTH - level - 1));
    uint64_t offset_in_page = shifted_page & (PAGE_SIZE - 1);

    PMread (frame_addr * PAGE_SIZE + offset_in_page, &frame_addr);
  }

  return frame_addr;
}

word_t get_last_frame (word_t start_frame, unsigned int current_level)
{
  if (current_level >= TABLES_DEPTH)
  {
    return 0;
  }

  word_t latest_frame_used = 0;
  word_t val;

  for (int offset = 0; offset < PAGE_SIZE; offset++)
  {
    PMread (start_frame * PAGE_SIZE + offset, &val);
    latest_frame_used = val < latest_frame_used ? latest_frame_used : val;

    val = get_last_frame (val, current_level + 1);
    latest_frame_used = val < latest_frame_used ? latest_frame_used : val;
  }

  return latest_frame_used;
}

word_t compute_cyclical_distance (word_t start_frame,
                                  word_t prior_frames[TABLES_DEPTH],
                                  word_t input_page, word_t cur_page,
                                  word_t *selected_victim, unsigned int level)
{
  if (level >= TABLES_DEPTH)
  {
    if (check_prior_frame (retrieve_frame (cur_page), prior_frames))
    {
      return 0;
    }

    *selected_victim = cur_page;
    word_t dist;
    if (input_page - cur_page >= 0)
    {
      dist = input_page - cur_page;
    }
    else
    {
      dist = cur_page - input_page;
    }

    if (NUM_PAGES - dist >= dist)
    {
      return dist;
    }
    else
    {
      return NUM_PAGES - dist;
    }

  }

  word_t furthest = 0;
  word_t val;

  for (int offset = 0; offset < PAGE_SIZE; offset++)
  {
    PMread (start_frame * PAGE_SIZE + offset, &val);

    if (val == 0)
    {
      continue;
    }
    word_t next_page = (cur_page << OFFSET_WIDTH) | offset;
    word_t next_selected_victim;

    val = compute_cyclical_distance (val, prior_frames,
                                     input_page, next_page,
                                     &next_selected_victim, level + 1);
    if (val > furthest)
    {
      furthest = val;
      *selected_victim = next_selected_victim;
    }
  }

  return furthest;
}

word_t get_unused_frame_of_process (word_t prior_frames[TABLES_DEPTH])
{
  word_t check = ERROR;

  for (word_t frame = 1; frame < NUM_FRAMES; frame++)
  {
    if (check_prior_frame (frame, prior_frames))
    {
      continue;
    }
    int counter = 0;
    word_t val;

    for (int offset = 0; offset < PAGE_SIZE; offset++)
    {
      PMread (frame * PAGE_SIZE + offset, &val);
      if (val == 0)
      {
        counter += 1;
      }
      else
      {
        counter += 0;
      }
    }

    if (!check_if_last_frame (0, frame, 0) && counter == PAGE_SIZE)
    {
      check = frame;
      break;
    }
  }

  if (check != ERROR)
  {
    delete_connection (0, check, 0);

    return check;

  }
  return ERROR;
}

word_t find_free_frame ()
{
  word_t useable_frame = get_last_frame (0, 0) + 1;

  if (useable_frame < NUM_FRAMES)
  {
    return useable_frame;
  }

  return ERROR;
}

word_t evict_cyclical_victim (word_t input_page,
                              word_t prior_frames[TABLES_DEPTH])
{
  word_t victim;
  compute_cyclical_distance (0, prior_frames, input_page, 0, &victim, 0);

  return victim;
}

word_t translate_address (uint64_t v_address, int levels)
{
  if ((v_address >> VIRTUAL_ADDRESS_WIDTH) > 0)
  {
    return ERROR;
  }
  word_t cur_frame = 0;
  uint64_t page_num = v_address >> OFFSET_WIDTH;
  word_t previous_frames[TABLES_DEPTH] = {0};

  for (int level = 0; level < levels; ++level)
  {
    uint64_t idx = v_address >> ((levels - level) * OFFSET_WIDTH);
    uint64_t offset = (PAGE_SIZE - 1) & idx;

    word_t prev_frame = cur_frame;
    PMread (PAGE_SIZE * cur_frame + offset, &cur_frame);
    if (cur_frame != 0)
    {
      // do nothing, cur_frame is already set
    }
    else
    {
      word_t frame = locate_frame_for_insertion (page_num, previous_frames);

      if (frame == ERROR)
      {
        return ERROR;
      }

      PMwrite (prev_frame * PAGE_SIZE + offset, frame);

      if (level == TABLES_DEPTH - 1)
      {
        PMrestore (frame, idx);
        cur_frame = frame;
      }

      else if (level < TABLES_DEPTH - 1)
      {
        clear_frame (frame);
        cur_frame = frame;
      }

    }

    previous_frames[level] = cur_frame;
  }

  return cur_frame;
}

word_t locate_frame_for_insertion (word_t input_page,
                                   word_t previous_frames[TABLES_DEPTH])
{
  word_t frame;

  // first prio
  frame = get_unused_frame_of_process (previous_frames);

  if (frame != ERROR)
  {
    return frame;
  }

  // second prio
  frame = find_free_frame ();

  if (frame != ERROR)
  {
    return frame;
  }

  // third prio
  word_t page_to_evict = evict_cyclical_victim (input_page, previous_frames);
  frame = retrieve_frame (page_to_evict);

  PMevict (frame, page_to_evict);

  word_t new_address = translate_address (page_to_evict, TABLES_DEPTH - 1);
  uint64_t offset_within_page = page_to_evict & (PAGE_SIZE - 1);
  PMwrite (new_address * PAGE_SIZE + offset_within_page, 0);

  return frame;
}

void VMinitialize ()
{
  clear_frame (0);
}

int VMread (uint64_t virtualAddress, word_t *value)
{

  if (value == nullptr)
  {
    return 0;
  }
  if (TABLES_DEPTH != 0)
  {

    word_t frame_start_address = translate_address (virtualAddress, TABLES_DEPTH);

    if (frame_start_address == ERROR)
    {
      return 0;
    }

    uint64_t offset = virtualAddress & (PAGE_SIZE - 1);

    PMread (PAGE_SIZE * frame_start_address + offset, value);
    return 1;
  }
  PMread (virtualAddress, value);
  return 1;
}

int VMwrite (uint64_t virtualAddress, word_t value)
{
  if (TABLES_DEPTH != 0)
  {

    word_t frame_start_address = translate_address (virtualAddress, TABLES_DEPTH);

    if (frame_start_address == ERROR)
    {
      return 0;
    }

    uint64_t offset = virtualAddress & (PAGE_SIZE - 1);

    PMwrite (frame_start_address * PAGE_SIZE + offset, value);
    return 1;
  }
  PMwrite (virtualAddress, value);
  return 1;
}


