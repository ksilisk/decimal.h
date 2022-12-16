#include "s21_decimal.h"

void print_decimal(s21_decimal number) {
  for (int i = 0; i < 4; i++) {
    char binary_number[33];

    for (int j = 0; j < 32; j++) {
      binary_number[31 - j] = (char)(((number.bits[i] >> j) & 1u) + 48);
    }

    binary_number[32] = '\0';
    printf("[%s]", binary_number);
  }

  printf("\n");
}

void s21_shift_left(s21_decimal *number) {
  int low_last_bit = s21_get_bit(*number, 31);
  int mid_last_bit = s21_get_bit(*number, 63);

  number->bits[LOW] <<= 1;
  number->bits[MID] <<= 1;
  number->bits[HIGH] <<= 1;

  s21_set_bit(&number->bits[MID], 0, low_last_bit);
  s21_set_bit(&number->bits[HIGH], 0, mid_last_bit);
}

void s21_shift_right(s21_decimal *number) {
  int mid_first_bit = s21_get_bit(*number, 32);
  int high_first_bit = s21_get_bit(*number, 64);

  number->bits[LOW] >>= 1;
  number->bits[MID] >>= 1;
  number->bits[HIGH] >>= 1;

  s21_set_bit(&number->bits[MID], 31, high_first_bit);
  s21_set_bit(&number->bits[LOW], 31, mid_first_bit);
}

int s21_get_bit(s21_decimal number, int bit_position) {
  int bit_number = bit_position / 32;
  bit_position = bit_position % 32;
  int bit = (int)((number.bits[bit_number] >> bit_position) & 1u);

  return bit;
}

int s21_get_highest_bit(s21_decimal number) {
  int bit_position = -1;

  for (int i = 0; i < 96 && bit_position == -1; i++) {
    if (s21_get_bit(number, (95 - i)) == 1) {
      bit_position = (95 - i);
    }
  }

  return bit_position;
}

void s21_set_bit(unsigned int *source_number, int bit_position, int bit) {
  unsigned int mask = 1;
  mask <<= bit_position;

  if (bit == 0) {
    mask = ~mask;
    *source_number &= mask;
  }

  if (bit == 1) {
    *source_number |= mask;
  }
}

int s21_get_sign(s21_decimal number) {
  int sign = (int)((number.bits[SCALE] >> 31) & 1u);

  return sign;
}

void s21_set_sign(s21_decimal *number, int sign) {
  if (sign == 0) {
    number->bits[SCALE] <<= 1;
    number->bits[SCALE] >>= 1;
  }
  if (sign == 1) {
    int mask = 0b10000000000000000000000000000000;
    number->bits[SCALE] = number->bits[SCALE] | mask;
  }
}

int s21_get_scale(s21_decimal number) {
  int scale = 0;
  number.bits[SCALE] >>= 16;

  for (int j = 0; j < 8; j++) {
    scale += (int)(((number.bits[SCALE] >> j) & 1u) * (int)pow(2, j));
  }

  return scale;
}

void s21_set_scale(s21_decimal *number, int scale) {
  int sign = s21_get_sign(*number);

  number->bits[SCALE] = scale;
  number->bits[SCALE] <<= 16;

  if (sign == 1) {
    s21_set_sign(number, 1);
  }
}

void s21_equalize_scale(s21_decimal *number1, s21_decimal *number2) {
  int number1_scale = s21_get_scale(*number1);
  int number2_scale = s21_get_scale(*number2);
  int scale_diff = number1_scale - number2_scale;

  if (scale_diff < 0) {
    scale_diff *= -1;
  }

  if (scale_diff != 0) {
    s21_decimal *bigger_scale_number;
    s21_decimal *smaller_scale_number;

    if (number1_scale > number2_scale) {
      bigger_scale_number = number1;
      smaller_scale_number = number2;
    } else {
      bigger_scale_number = number2;
      smaller_scale_number = number1;
    }

    for (int i = 0; i < scale_diff; i++) {
      if (s21_is_multiply_possible(*smaller_scale_number) == 1) {
        int current_scale = s21_get_scale(*smaller_scale_number);

        s21_multiply_by_10(smaller_scale_number);
        s21_set_scale(smaller_scale_number, current_scale + 1);
      } else {
        int current_scale = s21_get_scale(*bigger_scale_number);

        if (i == scale_diff - 1) {
          unsigned int last = bigger_scale_number->bits[LOW] % 10;
          unsigned int penultimate =
              bigger_scale_number->bits[LOW] % 100 - last;
          int bigger_number_sign = s21_get_sign(*bigger_scale_number);

          s21_divide_by_10(bigger_scale_number);
          s21_set_scale(bigger_scale_number, current_scale - 1);

          if ((last == 5) && (penultimate % 2) == 1) {
            s21_decimal result = {{0, 0, 0, 0}};

            if (bigger_number_sign == 0) {
              s21_decimal s21_one = {{1, 0, 0, 0}};
              s21_add_without_scale(*bigger_scale_number, s21_one, &result);

              *bigger_scale_number = result;
            } else {
              s21_decimal s21_one = {{1, 0, 0, 0}};
              s21_set_sign(&s21_one, 1);
              s21_add_without_scale(*bigger_scale_number, s21_one, &result);

              *bigger_scale_number = result;
            }
          }
        } else {
          s21_divide_by_10(bigger_scale_number);
          s21_set_scale(bigger_scale_number, current_scale - 1);
        }
      }
    }
  }
}

int s21_is_multiply_possible(s21_decimal number) {
  int is_multiply_possible = 0;
  int bit_29 = s21_get_bit(number, 93);
  int bit_30 = s21_get_bit(number, 94);
  int bit_31 = s21_get_bit(number, 95);

  if (bit_29 == 0 && bit_30 == 0 && bit_31 == 0) {
    s21_decimal number_multiplied_by_8 = number;
    s21_shift_left(&number_multiplied_by_8);
    s21_shift_left(&number_multiplied_by_8);
    s21_shift_left(&number_multiplied_by_8);

    s21_decimal number_multiplied_by_2 = number;
    s21_shift_left(&number_multiplied_by_2);

    s21_decimal result = {{0, 0, 0, 0}};

    if (s21_add_without_scale(number_multiplied_by_8, number_multiplied_by_2,
                              &result) == 0) {
      is_multiply_possible = 1;
    }
  }

  return is_multiply_possible;
}

void s21_multiply_by_10(s21_decimal *number) {
  s21_decimal number_multiplied_by_8 = *number;
  s21_shift_left(&number_multiplied_by_8);
  s21_shift_left(&number_multiplied_by_8);
  s21_shift_left(&number_multiplied_by_8);

  s21_decimal number_multiplied_by_2 = *number;
  s21_shift_left(&number_multiplied_by_2);

  s21_add_without_scale(number_multiplied_by_8, number_multiplied_by_2, number);
}

int s21_add_without_scale(s21_decimal value_1, s21_decimal value_2,
                          s21_decimal *result) {
  int is_overflow = 0;
  unsigned int lowest_number = value_1.bits[LOW];

  if (value_1.bits[LOW] > value_2.bits[LOW]) {
    lowest_number = value_2.bits[LOW];
  }
  if (value_1.bits[LOW] + value_2.bits[LOW] < lowest_number) {
    result->bits[MID] = 1u;
  } else {
    result->bits[MID] = 0;
  }
  result->bits[LOW] = value_1.bits[LOW] + value_2.bits[LOW];
  lowest_number = value_1.bits[MID];

  if (value_1.bits[MID] > value_2.bits[MID]) {
    lowest_number = value_2.bits[MID];
  }

  if (value_1.bits[MID] + value_2.bits[MID] + result->bits[MID] <
      lowest_number) {
    result->bits[HIGH] = 1u;
  } else {
    result->bits[HIGH] = 0;
  }
  result->bits[MID] += value_1.bits[MID] + value_2.bits[MID];
  lowest_number = value_1.bits[HIGH];

  if (value_1.bits[HIGH] > value_2.bits[HIGH]) {
    lowest_number = value_2.bits[HIGH];
  }
  if (value_1.bits[HIGH] + value_2.bits[HIGH] + result->bits[HIGH] <
      lowest_number) {
    result->bits[LOW] = 0;
    result->bits[MID] = 0;
    result->bits[HIGH] = 0;
    result->bits[SCALE] = 0;
    is_overflow = 1;
  } else {
    result->bits[HIGH] += value_1.bits[HIGH] + value_2.bits[HIGH];
  }

  return is_overflow;
}

unsigned int s21_divide_by_10(s21_decimal *number) {
  unsigned int buffer = 0;
  int scale_of_number = s21_get_scale(*number);
  s21_decimal result = {{0, 0, 0, 0}};

  int highest_bit_position = s21_get_highest_bit(*number);

  for (int i = 0; i < highest_bit_position + 2; i++) {
    if (buffer >= 10) {
      s21_shift_left(&result);
      s21_set_bit(&result.bits[LOW], 0, 1);
      buffer = buffer - 10u;
      int new_bit = s21_get_bit(*number, highest_bit_position - i);
      buffer = buffer << 1;
      s21_set_bit(&buffer, 0, new_bit);
    } else {
      s21_shift_left(&result);
      s21_set_bit(&result.bits[LOW], 0, 0);
      int new_bit = s21_get_bit(*number, highest_bit_position - i);
      buffer = buffer << 1;
      s21_set_bit(&buffer, 0, new_bit);
    }
  }

  buffer = buffer >> 1;
  *number = result;
  s21_set_scale(number, scale_of_number);

  return buffer;
}
