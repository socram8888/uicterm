
#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct telegram telegram_t;

typedef enum {
	/**
	 * The telegram contains valid values which can be fetched using telegram_get_* methods.
	 */
	TELEGRAM_OK,

	/**
	 * The telegram needs more bits to be fed.
	 */
	TELEGRAM_MORE,

	/**
	 * The telegram has an invalid synchronization header
	 */
	TELEGRAM_NO_SYNC,

	/**
	 * The telegram has a failed the integrity test
	 */
	TELEGRAM_INTEGRITY,
} telegram_status_t;

/**
 * Creates a new telegram parser
 *
 * @returns New telegram object, or NULL on error
 */
telegram_t * telegram_init();

/**
 * Returns current telegram status
 *
 * @param t Telegram object
 * @returns Current telegram status
 */
telegram_status_t telegram_status(telegram_t * t);

/**
 * Returns true if the telegram is done being received
 *
 * @param t Telegram object
 * @returns Current telegram status
 */
bool telegram_is_done(telegram_t * t);

/**
 * Returns telegram train number. Return value is only valid if current
 * telegram is done.
 *
 * @param t Telegram object
 * @returns Telegram train ID
 */
int telegram_train_number(telegram_t * t);

/**
 * Returns telegram code. Return value is only valid if current telegram is
 * done.
 *
 * @param t Telegram object
 * @returns Telegram code
 */
int telegram_code_number(telegram_t * t);

/**
 * Returns the received telegram CRC. Return value is only valid if current
 * telegram is done.
 *
 * @param t Telegram object
 * @returns Telegram CRC
 */
int telegram_received_crc(telegram_t * t);

/**
 * Returns the correct telegram CRC. Return value is only valid if current
 * telegram is done.
 *
 * @param t Telegram object
 * @returns Telegram CRC
 */
int telegram_correct_crc(telegram_t * t);

/**
 * Returns the raw telegram bits. Return value is only valid if current
 * telegram is done.
 *
 * @param t Telegram object
 * @returns Telegram bits
 */
int64_t telegram_raw(telegram_t * t);

/**
 * Feeds a new bit to the telegram object
 *
 * @param t Telegram object
 * @param bit Input bit
 */
void telegram_feed(telegram_t * t, int bit);

/**
 * Resets the telegram status and bit buffer
 *
 * @param t Telegram object
 */
void telegram_reset(telegram_t * t);

/**
 * Destroys the telegram object. Accepts NULL.
 *
 * @param t Telegram object
 */
void telegram_free(telegram_t * t);
