// if s1 starts with s2 returns true, else false
// len is the length of s1
// s2 should be null-terminated
static bool starts_with(const char *s1, int len, const char *s2)
{
  int n = 0;
  while (*s2 && n < len) {
    if (*s1++ != *s2++)
      return false;
    n++;
  }
  return *s2 == 0;
}

#define FOO(...) { \
  FILE* f = fopen("log", "a+"); \
  fprintf(f, __VA_ARGS__); \
  fclose(f); \
}

// convert escape sequence to event, and return consumed bytes on success (failure == 0)
static int parse_escape_seq(struct tb_event *event, const char *buf, int len)
{
  static int parse_attempts = 0;
  static const int MAX_PARSE_ATTEMPTS = 2;

//?   int x = 0;
//?   FOO("-- %d\n", len);
//?   for (x = 0; x < len; ++x) {
//?     FOO("%d\n", (unsigned char)buf[x]);
//?   }
  if (len >= 6 && starts_with(buf, len, "\033[M")) {

    switch (buf[3] & 3) {
    case 0:
      if (buf[3] == 0x60)
        event->key = TB_KEY_MOUSE_WHEEL_UP;
      else
        event->key = TB_KEY_MOUSE_LEFT;
      break;
    case 1:
      if (buf[3] == 0x61)
        event->key = TB_KEY_MOUSE_WHEEL_DOWN;
      else
        event->key = TB_KEY_MOUSE_MIDDLE;
      break;
    case 2:
      event->key = TB_KEY_MOUSE_RIGHT;
      break;
    case 3:
      event->key = TB_KEY_MOUSE_RELEASE;
      break;
    default:
      parse_attempts = 0;
      return -6;
    }
    event->type = TB_EVENT_MOUSE; // TB_EVENT_KEY by default

    // the coord is 1,1 for upper left
    event->x = (uint8_t)buf[4] - 1 - 32;
    event->y = (uint8_t)buf[5] - 1 - 32;

    parse_attempts = 0;
    return 6;
  }

  // it's pretty simple here, find 'starts_with' match and return
  // success, else return failure
  int i;
  for (i = 0; keys[i]; i++) {
    if (starts_with(buf, len, keys[i])) {
      event->ch = 0;
      event->key = 0xFFFF-i;
      parse_attempts = 0;
      return strlen(keys[i]);
    }
  }

  if (starts_with(buf, len, "\033[200~")) {
    event->ch = 0;
    event->key = TB_KEY_START_PASTE;
    parse_attempts = 0;
    return strlen("\033[200~");
  }
  if (starts_with(buf, len, "\033[201~")) {
    event->ch = 0;
    event->key = TB_KEY_END_PASTE;
    parse_attempts = 0;
    return strlen("\033[201~");
  }
  if (starts_with(buf, len, "\033[1;5A")) {
    event->ch = 0;
    event->key = TB_KEY_CTRL_ARROW_UP;
    parse_attempts = 0;
    return strlen("\033[1;5A");
  }
  if (starts_with(buf, len, "\033[1;5B")) {
    event->ch = 0;
    event->key = TB_KEY_CTRL_ARROW_DOWN;
    parse_attempts = 0;
    return strlen("\033[1;5B");
  }
  if (starts_with(buf, len, "\033[1;5C")) {
    event->ch = 0;
    event->key = TB_KEY_CTRL_ARROW_RIGHT;
    parse_attempts = 0;
    return strlen("\033[1;5C");
  }
  if (starts_with(buf, len, "\033[1;5D")) {
    event->ch = 0;
    event->key = TB_KEY_CTRL_ARROW_LEFT;
    parse_attempts = 0;
    return strlen("\033[1;5D");
  }
  if (starts_with(buf, len, "\033[Z")) {
    event->ch = 0;
    event->key = TB_KEY_SHIFT_TAB;
    parse_attempts = 0;
    return strlen("\033[Z");
  }

  // no escape sequence recognized? wait a bit in case our buffer is incomplete
  ++parse_attempts;
  if (parse_attempts < MAX_PARSE_ATTEMPTS) return 0;
  // still nothing? give up and consume just the esc
  event->ch = 0;
  event->key = TB_KEY_ESC;
  parse_attempts = 0;
  return 1;
}

static bool extract_event(struct tb_event *event, struct bytebuffer *inbuf)
{
  const char *buf = inbuf->buf;
  const int len = inbuf->len;
  if (len == 0)
    return false;

//?   int x = 0;
//?   FOO("== %d\n", len);
//?   for (x = 0; x < len; ++x) {
//?     FOO("%x\n", (unsigned char)buf[x]);
//?   }
  if (buf[0] == '\033') {
    int n = parse_escape_seq(event, buf, len);
    if (n == 0) return false;
//?     FOO("parsed: %u %u %u %u\n", n, (unsigned int)event->type, (unsigned int)event->key, event->ch);
    bool success = true;
    if (n < 0) {
      success = false;
      n = -n;
    }
    bytebuffer_truncate(inbuf, n);
    return success;
  }

  // if we're here, this is not an escape sequence and not an alt sequence
  // so, it's a FUNCTIONAL KEY or a UNICODE character

  // first of all check if it's a functional key
  if ((unsigned char)buf[0] <= TB_KEY_SPACE ||
      (unsigned char)buf[0] == TB_KEY_BACKSPACE2)
  {
    // fill event, pop buffer, return success */
    event->ch = 0;
    event->key = (uint16_t)buf[0];
    bytebuffer_truncate(inbuf, 1);
    return true;
  }

  // feh... we got utf8 here

  // check if there is all bytes
  if (len >= tb_utf8_char_length(buf[0])) {
    /* everything ok, fill event, pop buffer, return success */
    tb_utf8_char_to_unicode(&event->ch, buf);
    event->key = 0;
    bytebuffer_truncate(inbuf, tb_utf8_char_length(buf[0]));
    return true;
  }

  // event isn't recognized, perhaps there is not enough bytes in utf8
  // sequence
  return false;
}
