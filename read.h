#ifndef READ_H_INCLUDED
#define READ_H_INCLUDED

// Read tokens. Returns true iff okay.
bool read(const char * const filename,
          struct Tokens & tokens, struct Comments & comments);

#endif // READ_H_INCLUDED
