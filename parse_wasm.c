#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

enum
{
  TYPE_I32 = 0x7f,
  TYPE_I64 = 0x7e,
  TYPE_F32 = 0x7d,
  TYPE_F64 = 0x7c,
  TYPE_ANYFUNC = 0x70,
  TYPE_FUNC = 0x60,
  TYPE_BLOCK_TYPE = 0x40,
};

static const char *section_names[] =
{
  "",
  "Type",      // 1
  "Import",    // 2
  "Function",  // 3
  "Table",     // 4
  "Memory",    // 5
  "Global",    // 6
  "Export",    // 7
  "Start",     // 8
  "Element",   // 9
  "Code",      // 10
  "Data",      // 11
};

static int read_uint(FILE *in, uint32_t *n, int len)
{
  int ch;
  int shift = 0;
  int i;

  *n = 0;

  for (i = 0; i < len; i++)
  {
    ch = getc(in);

    if (ch == EOF)
    {
      printf("Error: Premature end of file.\n");
      return -1;
    }

    *n |= ch << shift;
    shift += 8;
  }

  return 0;
}

static int read_varuint(FILE *in, uint32_t *n, int *len)
{
  int ch;
  int shift = 0;
  int done = 0;

  *len = 0;
  *n = 0;

  while (done == 0)
  {
    ch = getc(in);

    if (ch == EOF)
    {
      printf("Error: Premature end of file.\n");
      return -1;
    }

    *len += 1;

    if ((ch & 0x80) == 0)
    {
      done = 1;
    }

    ch = ch & 0x7f;

    *n |= ch << shift;
    shift += 7;
  }

  return 0;
}

#if 0
static int read_varint(FILE *in, int32_t *n, int *len)
{
  int ch;
  int shift = 0;
  int done = 0;
  int num = 0;

  *len = 0;

  while (done == 0)
  {
    ch = getc(in);

    if (ch == EOF)
    {
      printf("Error: Premature end of file.\n");
      return -1;
    }

    *len += 1;

    if ((ch & 0x80) == 0)
    {
      done = 1;
    }

    ch = ch & 0x7f;

    num |= ch << shift;
    shift += 7;
  }

  if ((num & (1 << (shift - 1))) != 0)
  {
    num |= ~((1 << shift) - 1);
  }

  *n = num;

  return 0;
}
#endif

static int print_string(FILE *in, int len)
{
  int n, ch;

  for (n = 0; n < len; n++)
  {
    ch = getc(in);
    if (ch == EOF) { return -1; }

    printf("%c", ch);
  }

  printf("\n");

  return 0;
}

static int print_header(FILE *in)
{
  char magic_number[4];
  uint32_t version;

  if (fread(magic_number, 1, 4, in) != 4 ||
      memcmp(magic_number, "\0asm", 4) != 0)
  {
    printf("Error: Bad magic_number, not wasm file.\n");
    return -1;
  }

  if (read_uint(in, &version, 4) != 0) { return -1; }

  printf("magic_number: %02x %c%c%c\n",
    magic_number[0],
    magic_number[1],
    magic_number[2],
    magic_number[3]);
  printf("     version: %d\n", version);

  return 0;
}

static const char *get_type(int type)
{
  switch (type)
  {
    case TYPE_I32: return "i32";
    case TYPE_I64: return "i64";
    case TYPE_F32: return "f32";
    case TYPE_F64: return "f64";
    case TYPE_ANYFUNC: return "anyfunc";
    case TYPE_FUNC: return "func";
    case TYPE_BLOCK_TYPE: return "<pseudo type>";
    default: return "?";
  }
}

static const char *get_kind(int kind)
{
  const char *kinds[] = { "Function", "Table", "Memory", "Global" };

  if (kind < 0 || kind > 3) { return ""; }

  return kinds[kind];
}

static int print_section_type(FILE *in)
{
  int len;
  uint32_t count;
  uint32_t type, param_count, param_type, return_count, return_type;
  uint32_t n, i;

  printf("\n");

  // What I did here contradicts the documentation I found online
  // on the wasm file format.  What they posted doesn't appear to
  // work and based on what's in the file, this makes sense.

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    printf("      --- index %d ---\n", n);
    if (read_varuint(in, &type, &len) != 0) { return -1; }

    printf("         type: %02x %s\n", type, get_type(type));

    if (type != 0x60) { continue; }

    if (read_varuint(in, &param_count, &len) != 0) { return -1; }

    printf("  param_count: %d\n", param_count);
    printf("       params:");

    for (i = 0; i < param_count; i++)
    {
      if (read_varuint(in, &param_type, &len) != 0) { return -1; }
      printf(" %s", get_type(param_type));
    }

    printf(" returns:");

    if (read_varuint(in, &return_count, &len) != 0) { return -1; }

    for (n = 0; n < return_count; n++)
    {
      if (read_varuint(in, &return_type, &len) != 0) { return -1; }
      printf(" %s", get_type(return_type));
    }

    printf("\n");
  }

  return 0;
}

#if 0
static int print_section_import(FILE *in)
{
}
#endif

static int print_section_function(FILE *in)
{
  int len, n;
  uint32_t count, index;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    if (read_varuint(in, &index, &len) != 0) { return -1; }
    printf("       %d) type_index=%d\n", n, index);
  }

  return 0;
}

static int print_section_table(FILE *in)
{
  int len, n;
  uint32_t count, type, flags, initial;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    printf("      --- index %d ---\n", n);
    if (read_varuint(in, &type, &len) != 0) { return -1; }
    if (read_varuint(in, &flags, &len) != 0) { return -1; }
    if (read_varuint(in, &initial, &len) != 0) { return -1; }

    // I think something is missing here.
    printf("         type: %02x %s\n", type, get_type(type));
    printf("        flags: %02x\n", flags);
    printf("      initial: %d\n", initial);
  }

  return 0;
}

static int print_section_memory(FILE *in)
{
  int len, n;
  uint32_t count, flags, initial;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    printf("      --- index %d ---\n", n);
    if (read_varuint(in, &flags, &len) != 0) { return -1; }
    if (read_varuint(in, &initial, &len) != 0) { return -1; }

    // I think something is missing here.
    printf("        flags: %02x\n", flags);
    printf("      initial: %d\n", initial);
  }

  return 0;
}

static int print_section_global(FILE *in)
{
  int len, n;
  uint32_t count;
  uint32_t field_len;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    printf("      --- index %d ---\n", n);

    if (read_varuint(in, &field_len, &len) != 0) { return -1; }

    printf("        field_len: %d\n", field_len);


    // FIXME: This requires webasm some code.
  }

  return 0;
}

static int print_section_export(FILE *in)
{
  int len, n;
  uint32_t count, kind, index, field_len;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    if (read_varuint(in, &field_len, &len) != 0) { return -1; }

    printf("   field_len: %d\n", field_len);
    printf("       field: ");
    print_string(in, field_len);

    if (read_varuint(in, &kind, &len) != 0) { return -1; }
    if (read_varuint(in, &index, &len) != 0) { return -1; }

    printf("         kind: %02x %s\n", kind, get_kind(kind));
    printf("        index: %d\n", index);
  }

  return 0;
}

static int print_section_code(FILE *in)
{
  int len, n, i, ch;
  uint32_t count, body_size, local_count, local_entry;

  if (read_varuint(in, &count, &len) != 0) { return -1; }

  printf("\n");
  printf("      count: %d\n", count);

  for (n = 0; n < count; n++)
  {
    if (read_varuint(in, &body_size, &len) != 0) { return -1; }
    if (read_varuint(in, &local_count, &len) != 0) { return -1; }

    printf("   body_size: %d\n", body_size);
    printf(" local_count: %d\n", local_count);
    printf("      locals:");

    body_size -= len;

    for (i = 0; i < local_count; i++)
    {
      if (read_varuint(in, &local_entry, &len) != 0) { return -1; }

      body_size -= len;

      printf(" %s", get_type(local_entry));
    }

    printf("\n");
    printf("        code:");

    for (i = 0; i < body_size; i++)
    {
      ch = getc(in);

      if (ch == EOF)
      {
        printf("Error: Premature end of file.\n");
        return -1;
      }

      printf(" %02x", ch);
    }

    printf("\n");
  }

  return 0;
}

static int print_section(FILE *in)
{
  int len, n, ch;
  uint32_t id;
  uint32_t payload_len;
  //uint32_t name_len;
  const char *section_name = "";

  printf("--- SECTION (offset=0x%04x) ---\n", (int)ftell(in));

  if (read_varuint(in, &id, &len) != 0) { return -1; }
  if (read_varuint(in, &payload_len, &len) != 0) { return -1; }

  if (id <= 11) { section_name = section_names[id]; }

  printf("         id: %d (%s)\n", id, section_name);
  printf("payload_len: %d\n", payload_len);

#if 0
  if (read_varuint(in, &name_len, &len) != 0) { return -1; }
  payload_len -= len + name_len;

  printf("   name_len: %d\n", name_len);
  printf("       name:");
  print_string(in, len);
#endif

  printf("    payload:");

  switch (id)
  {
    case 1:
      print_section_type(in);
      break;
    //case 2:
    //  print_section_import(in);
    //  break;
    case 3:
      print_section_function(in);
      break;
    case 4:
      print_section_table(in);
      break;
    case 5:
      print_section_memory(in);
      break;
    case 6:
      print_section_global(in);
      break;
    case 7:
      print_section_export(in);
      break;
    case 10:
      print_section_code(in);
      break;
    default:
    {
      for (n = 0; n < payload_len; n++)
      {
        ch = getc(in);

        if (ch == EOF)
        {
          printf("Error: Premature end of file.\n");
          return -1;
        }

        printf(" %02x", ch);
      }

      printf("\n");

      break;
    }
  } 

  return 0;
}

int main(int argc, char *argv[])
{
  FILE *in;

  if (argc != 2)
  {
    printf("Usage: %s <filename.wasm>\n", argv[0]);
    exit(0);
  }

  in = fopen(argv[1], "rb");

  if (in == NULL)
  {
    printf("Error: Could not open file for reading %s\n", argv[0]);
    exit(1);
  }

  fseek(in, 0, SEEK_END);
  long length = ftell(in);
  fseek(in, 0, SEEK_SET);

  if (print_header(in) == 0)
  {
    while (ftell(in) < length)
    {
      if (print_section(in) != 0) { break; }
    }
  }

  fclose(in);

  return 0;
}

