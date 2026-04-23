//
// Bested.h - Best Standard.h
//
// Author   - Fletcher M
//
// Created  - 04/08/25
// Modified - 22/04/26
//
// Version  - 1.2.0
//
// Make sure to...
//      #define BESTED_IMPLEMENTATION
// ...somewhere in your project



//
//                        A Note On Namespaceing
//           (aka, why the Bested.h library is not namespaced)
//
// The normal way single header librarys in c are namespaced is by putting the
// name of the library at the start of all function and macros, like stb_xxx
// or in this case Bested_xxx.
//
// Some librarys even provide macros to strip the prefix of the functions and
// macros, for ease of use, for example the nob.h library has a
// NOB_STRIP_PREFIXES define, that turns functions like nob_cmd_append() into
// cmd_append().
//
// So why doesn't this single header library do the same thing?
//
// Well... its for a bit of a silly reason, my editor (VSCode, dont laugh),
// has a feature where if you hover over a identifier, it shows the definition
// of that identifier, as well as the comments above the identifier. this is
// extremely useful for when I'm inspecing my code.
//
// So if I where to pull the trick of having a BESTED_STRIP_PREFIXES (witch I
// would use all the time, as Bested.h is suppost to be for my ease of use)
// all of my cool comments, and function declarations in my tiny editor window
// would just be gone, and the feature would just point to the macro that
// strips the prefixes.
//
// A slight drop in developer ergonomics, that is compounded by the fact that
// most "go to definition" features would also be messed up by this.
// (Taking you to the macro again)
//
// So this library dose not do namespaceing.
//
// If this library collides with another library, figure it out yourself. Im
// not sacrificing my experience to deal with some theoretical namespace
// collision. The only collision I have encountered is the Clamp() macro with
// raylib's Clamp() function, witch was handled by #undef'ing Bested.h's
// Clamp(), its not that big of a deal.
//
// Also this library is free to modify, just change the function signature
// yourself.
//



#ifdef __cplusplus
    extern "C" {
#endif


#ifndef BESTED_H
#define BESTED_H



// ===================================================
//                    Includes
// ===================================================

// for my ints.
#include <stdint.h>
// for 'true' and 'false'
#include <stdbool.h>
// any real project is gonna need these two, for 'malloc' and 'printf'.
#include <stdlib.h>
#include <stdio.h>

#include <stddef.h> // for 'offsetof'
#include <stdalign.h> // for 'alignof'

#include <assert.h> // mainly for static_assert, i define my own ASSERT




// ===================================================
//                    Classic Ints
// ===================================================

typedef uint64_t        u64;
typedef uint32_t        u32;
typedef uint16_t        u16;
typedef uint8_t         u8;

typedef int64_t         s64;
typedef int32_t         s32;
typedef int16_t         s16;
typedef int8_t          s8;

// fixed width bool types, might be useful, but probably not.
typedef u64             b64;
typedef u32             b32;
typedef u16             b16;
typedef u8              b8;

typedef float           f32;
typedef double          f64;

// Turn your unknown size enums into known size enums
#define enum8(type)     u8
#define enum16(type)    u16
#define enum32(type)    u32
#define enum64(type)    u64



// ===================================================
//                    Nice Macros
// ===================================================

// control flow helper, useful to define more of these for more specific cases.
#define defer_return(res) do { result = (res); goto defer; } while (0)


// I always forget how to call typeof()
#define Typeof(x)       __typeof__(x)
// stick the extra Typeof() in there to prevent [-Wgnu-alignof-expression]
#define Alignof(x)      alignof(Typeof(x))


// Not wrapping stuff in () because this is purely text based, nothing fancy can be done here.
#define Member(type, member)        ( ((type*)0)->member )
#define Member_Size(type, member)   sizeof( Member(type, member) )


// offsetof is also here. comes from stddef.h


// I really hate c++ sometimes
#ifdef __cplusplus
    // this allows c++ to do value initialization,
    // witch is spiritually the same thing.
    //
    // also c++ complains if there is only 1 zero.
    #define ZEROED { /* Imagine there was a zero here */ }
#else
    // pretty sure this zero is necessary in C
    #define ZEROED {0}
#endif


#define Min(a, b)   ({ Typeof(a) _a = (a); Typeof(b) _b = (b); _a < _b ? _a : _b; })
#define Max(a, b)   ({ Typeof(a) _a = (a); Typeof(b) _b = (b); _a > _b ? _a : _b; })

#define Sign(T, x)  ((T)((x) > 0) - (T)((x) < 0))
#define Abs(x)      (Sign(Typeof(x), (x)) * (x))

// TODO this has the same problem as Min & Max
#define Clamp(x, min, max)              ((x) < (min) ? (min) : (((x) > (max)) ? (max) : (x)))

#define Is_Between(x, lower, upper)     (((lower) <= (x)) && ((x) <= (upper)))


#define Array_Len(array)            (sizeof(array) / sizeof((array)[0]))

// integers only
#define Is_Pow_2(n)                 (((n) != 0) && (((n) & ((n)-1)) == 0))

#define Proper_Mod(x, y) ({ Typeof(y) _y = (y); (((x) % _y) + _y) % _y; })

#define Div_Ceil(x, y)      (((x) + (y) - 1) / (y))
#define Div_Floor(x, y)     ((x) / (y))


#define Bit(n) (1 << (n))
#define Has_Bit(n, pos) ((n) & (1 << (pos)))


#define Flag_Set(n, f)          ((n) |= (f))
#define Flag_Clear(n, f)        ((n) &= ~(f))
#define Flag_Toggle(n, f)       ((n) ^= (f))
#define Flag_Exists(n, f)       (((n) & (f)) == (f))    // Checks if all bits in 'f' are set in 'n'
#define Flag_Equals(n, f)       ((n) == (f))            // Checks exact equality
#define Flag_Intersects(n, f)   (((n) & (f)) != 0)      // Checks if any bits in 'f' are set in 'n'



#define PI              3.1415926535897932384626433
#define TAU             6.283185307179586476925286766559
#define E               2.718281828459045235360287471352
#define ROOT_2          1.414213562373095048801688724209

#define KILOBYTE        (1024UL)
#define MEGABYTE        (1024UL * 1024UL)
#define GIGABYTE        (1024UL * 1024UL * 1024UL)
#define TERABYTE        (1024UL * 1024UL * 1024UL * 1024UL)

#define THOUSAND        (1000UL)
#define MILLION         (1000UL * 1000UL)
#define BILLION         (1000UL * 1000UL * 1000UL)
#define TRILLION        (1000UL * 1000UL * 1000UL * 1000UL)


#define MILISECONDS_PER_SECOND          THOUSAND
#define MICROSECONDS_PER_SECOND         MILLION
#define NANOSECONDS_PER_SECOND          BILLION

// good for usleep
#define MILISECONDS_PER_MICROSECOND     (MICROSECONDS_PER_SECOND / MILISECONDS_PER_SECOND)




// ===================================================
//         What static really means
// ===================================================

// Mark a function that must be used in the current compilation block and cannot be seen outside of it.
#define internal            static
// mark a variable inside of a function that persists though function calls.
#define local_persist       static
// casey once said that all this dose is help the compiler deal with
// multithreaded code, by forceing the functions to not store the
// value of the variable into their own registers.
#define global_variable     static



// ===================================================
//               Source Code Location
// ===================================================

// this is just pretty handy to carry around.
typedef struct {
    const char *file;
    s32 line;
} Source_Code_Location;

// printf helpers
#define SCL_Fmt         "%s:%d:"
#define SCL_Arg(scl)    scl.file, scl.line

#define Get_Source_Code_Location() ( (Source_Code_Location){ .file = __FILE__, .line = __LINE__ } )

bool source_code_location_eq(Source_Code_Location a, Source_Code_Location b);



// ===================================================
//         assert and aborting functions
// ===================================================

// this is probably not that good. kinda just a worse assert(),
// since i dont know how to get the pretty function text.
#define ASSERT(expr) do { if (!(expr)) {                                                    \
        Source_Code_Location scl = Get_Source_Code_Location();                              \
        fprintf(stderr, "===========================================\n");                   \
        fprintf(stderr, SCL_Fmt" ASSERTION ERROR: \"%s\"\n", SCL_Arg(scl), #expr);          \
        fprintf(stderr, "===========================================\n");                   \
        abort();                                                                            \
    } } while (0)

#define PANIC(message, ...) do {                                                            \
        Source_Code_Location scl = Get_Source_Code_Location();                              \
        fprintf(stderr, "===========================================\n");                   \
        fprintf(stderr, SCL_Fmt" PANIC: " message "\n", SCL_Arg(scl), ##__VA_ARGS__);       \
        fprintf(stderr, "===========================================\n");                   \
        abort();                                                                            \
    } while (0)

#define UNREACHABLE() do {                                                                  \
        Source_Code_Location scl = Get_Source_Code_Location();                              \
        fprintf(stderr, "===========================================\n");                   \
        fprintf(stderr, SCL_Fmt" UNREACHABLE\n", SCL_Arg(scl));                             \
        fprintf(stderr, "===========================================\n");                   \
        abort();                                                                            \
    } while (0)

#define TODO(message, ...) do {                                                             \
        Source_Code_Location scl = Get_Source_Code_Location();                              \
        fprintf(stderr, "===========================================\n");                   \
        fprintf(stderr, SCL_Fmt" TODO: " message "\n", SCL_Arg(scl), ##__VA_ARGS__);        \
        fprintf(stderr, "===========================================\n");                   \
        abort();                                                                            \
    } while (0)



// ===================================================
//        Useful Memory Manipulation Functions
// ===================================================

static_assert(sizeof(u64) >= sizeof(void*), "A pointer is pretty much always a u64");

#define Ptr_To_U64(ptr)     ((u64)(ptr))
// it doesn't matter if this looses any bits, if your doing
// this it was probably a pointer in the first place.
#define U64_To_Ptr(U64)     ((void*)(U64))

bool  Mem_Is_Aligned          (void *ptr, u64 alignment);
u64   Mem_Align_Forward       (u64 size, u64 alignment); // returns aligned value
u64   Mem_Align_Back          (u64 size, u64 alignment); // returns aligned value
void *Mem_Byte_Offset         (void *ptr, s64 bytes);
s64   Mem_Ptr_Diff            (void *ptr1, void *ptr2);  // ptr1 - ptr2

void *Mem_Copy(void *dest, void *src, u64 size);
void *Mem_Move(void *dest, void *src, u64 size);
void *Mem_Set (void *ptr, u8 value, u64 size);
s32   Mem_Cmp (void *ptr1, void *ptr2, u64 count);
#define Mem_Eq(ptr1, ptr2, count)   (Mem_Cmp((ptr1), (ptr2), (count)) == 0)
#define Mem_Zero(ptr, size)         Mem_Set((ptr), 0, (size))
#define Mem_Zero_Struct(x)          Mem_Zero((x), sizeof(*(x)))


#ifndef BESTED_ALIGNED_ALLOC
    #define BESTED_ALIGNED_ALLOC(align, size)       aligned_alloc((align), (size))
    #define BESTED_FREE(ptr)                        free(ptr)
#else
    #ifndef BESTED_FREE
        #error "Must Define BESTED_FREE as well as BESTED_ALIGNED_MALLOC"
    #endif
#endif // BESTED_ALIGNED_ALLOC

// this is always based on BESTED_ALIGNED_ALLOC,
// but no code will assert if BESTED_ALIGNED_ALLOC doesn't return the right alignment.
#define BESTED_MALLOC(size)                     BESTED_ALIGNED_ALLOC(Alignof(u64), size)



// ===================================================
//                      Atomics
// ===================================================

#ifdef __STDC_NO_ATOMICS__
    #error "This header uses atomics, could probably hide all the uses under a #define, if it was a problem"
#endif

#include <stdatomic.h>

#define Atomic(type) _Atomic(type)


// is it ever the case that a number may not be atomically set?
#define Atomic_Store(object, value)         atomic_store(object, value)
#define Atomic_Load(object)                 atomic_load(object)

// 'object' and 'expected' are pointers. 'desired' is just a value.
//
// on success, returns true,  and sets object   to desired
// on failure, returns false, and sets expected to object
//
// uses *_strong() because why not. (although *_weak() is faster in a loop apparently)
#define Atomic_Compare_And_Exchange(object, expected, desired)      atomic_compare_exchange_strong((object), (expected), (desired))

// all of these return the value they were before
#define Atomic_Exchange(object, operand)    atomic_exchange(object, operand)
#define Atomic_Add(object, operand)         atomic_fetch_add(object, operand)
#define Atomic_Sub(object, operand)         atomic_fetch_sub(object, operand)

#define Atomic_Or(object, operand)          atomic_fetch_or(object, operand)
#define Atomic_Xor(object, operand)         atomic_fetch_xor(object, operand)
#define Atomic_And(object, operand)         atomic_fetch_and(object, operand)

// A spin loop until the thread can capture the bool:
//      while (Atomic_Test_And_Set(object));
#define Atomic_Test_And_Set(object)         Atomic_Exchange(object, true)
#define Atomic_Clear(object)                Atomic_Store(object, false)

// capture a lock for the duration of some scope.
// this is made of 2 statements so dont put it right after an if statement or something...
//
// I shouldn't have to say this but NEVER try to escape the scope any other way then the bottom.
#define Atomic_Capture_Lock(lock) while (Atomic_Test_And_Set(lock)); for (int __lock_macro_holder = 0; __lock_macro_holder == 0; __lock_macro_holder = (Atomic_Clear(lock), 1))



// ===================================================
//                      Arena
// ===================================================


// if you provide a version of ARENA_PANIC that dose not abort(),
// we will return immediately after this macro is called.
#ifndef ARENA_PANIC
    #define ARENA_PANIC(scl, reason, ...)                                                               \
        do {                                                                                            \
            fprintf(stderr, "===========================================\n");                           \
            fprintf(stderr, SCL_Fmt" ARENA PANIC: \"" reason "\"\n", SCL_Arg(scl), ##__VA_ARGS__);      \
            fprintf(stderr, "===========================================\n");                           \
            abort();                                                                                    \
        } while(0)
#endif


#ifndef ARENA_REGION_DEFAULT_CAPACITY
    #define ARENA_REGION_DEFAULT_CAPACITY   (32 * KILOBYTE)
#endif // ARENA_REGION_DEFAULT_CAPACITY


//
// When you return a chunk of memory to the user, its important that the data is aligned to a
// register / pointer boundary. This means that when the user goes to read or write their ints
// or something, they dont have to do a stupid read across registers / memory locations, to get a single number.
// If you just did the most 'efficient' thing and alloc just the right amount of data and return the next section,
// you could not be aligned with a register boundary, and thus it would take a different kind of read by the CPU
// to get the memory to you, and to send it back down. This size should be something friendly to the CPU,
// (thats why we don't just use char or u8 here.)
//
// I use a 64-bit number here, (not just whatever the 'int' type is), so its easily known and mostly future proof.
// If you need 128-bit aligned pointers, just modify this file, or just only use the aligned parts,
// and waste a bit of memory, its fine to do so, this arena dose it all the time.
//
#define Default_Alignment   Alignof(u64)

typedef struct Region {
    struct Region *next;
    // just being explicit about bing in_bytes with these
    // two since my other arena is not in bytes.
    u64 count_in_bytes;
    u64 capacity_in_bytes;

    // used in Arena_Free()
    //
    // also this might be the only use of b32 ever. wow.
    b32 do_not_free_this;
    // extra padding bytes.
    u8 padding[4];

    u8 data[];
} Region;

// TODO do a thing more like the String_Builder, aka have arrays in a linked list.
typedef struct Arena {
    Region *first, *last;

    // how much to allocate when a new Region is required,
    // == 0: ARENA_REGION_DEFAULT_CAPACITY
    //  > 0: is how much to allocate, in bytes,
    u64 minimum_allocation_size;

    // TODO the following flags should be put into an enum flag.

    // If this is set to True, (anything thats not zero),
    // when performing allocating functions, (such as Arena_alloc),
    // it may instead return null when trying to allocate memory for a new Region,
    // instead of calling its panic function, (by default panic is just assert(false))
    //
    // You may want this if you want your programs to be bullet proof,
    // to not panic at the first sign of trouble.
    bool dont_panic_when_allocation_failure;

    // If this is set to True, if it runs out of memory in its current Region, it will panic.
    // Note. Arena_Initialize_First_Page() will still trigger this function.
    //
    // Uou may want this if you want more control over when your program allocates,
    // and want your program to have a fixed amount of memory usage.
    bool panic_when_trying_to_allocate_new_page;
} Arena;

// used for marking and then rewinding an arena.
typedef struct Arena_Mark {
    Region *last;
    u64 count;
} Arena_Mark;


typedef struct {
    u64 alignment;
    bool clear_to_zero;
} Arena_Alloc_Opt;


// Allocate some memory in a arena, uses macro tricks to give you more options.
void *_Arena_Alloc(Arena *arena, u64 size_in_bytes, Arena_Alloc_Opt opt, Source_Code_Location caller_location);

#define Arena_Alloc(arena, size, ...)      _Arena_Alloc((arena), (size), (Arena_Alloc_Opt){.alignment = Default_Alignment, .clear_to_zero = true, __VA_ARGS__ }, Get_Source_Code_Location())
#define Arena_Alloc_Struct(arena, type, ...)                         (type *)Arena_Alloc((arena), sizeof(type), .alignment = Alignof(type), ##__VA_ARGS__)





// get an arena's current position, to rewind it later.
Arena_Mark Arena_Get_Mark(Arena *arena);
// if the mark is not from this arena, your going to have a bad time.
void Arena_Set_To_Mark(Arena *arena, Arena_Mark mark);

// Will do nothing if the first page is already created.
//
// If 'first_page_size_in_bytes' is set to 0, the default is used, see 'minimum_allocation_size' comment above.
void _Arena_Initialize_First_Page(Arena *arena, u64 first_page_size_in_bytes, Source_Code_Location caller_location);
#define Arena_Initialize_First_Page(arena, first_page_size_in_bytes)        \
    _Arena_Initialize_First_Page((arena), (first_page_size_in_bytes), Get_Source_Code_Location());


// Care has been taken, so that when Arena_free is called,
// the pointer to the buffer provided here will not be free'd.
void _Arena_Add_Buffer_As_Storage_Space(Arena *arena, void *buffer, u64 buffer_size_in_bytes, Source_Code_Location caller_location);

#define Arena_Add_Buffer_As_Storage_Space(arena, buffer, buffer_size_in_bytes)     \
    _Arena_Add_Buffer_As_Storage_Space((arena), (buffer), (buffer_size_in_bytes), Get_Source_Code_Location())


// sprintf useing the arena as a buffer.
const char *Arena_sprintf(Arena *arena, const char *format, ...) __attribute__ ((format (printf, 2, 3)));

void Arena_Clear(Arena *arena);
void Arena_Free (Arena *arena);



// ===================================================
//                      Pool
// ===================================================

typedef u32 Pool_Flag_Type;
#define NUM_POOL_ARENAS (sizeof(Pool_Flag_Type) * 8)

//
// TODO this struct should have default arena settings,
// but how would this work if you wanted
// to give different arena's different settings?
//
typedef struct Arena_Pool {
    Atomic(Pool_Flag_Type)  in_use_flags;
    Atomic(bool)            creating_new_pool_in_chain_lock;

    Arena arena_pool[NUM_POOL_ARENAS];

    struct Arena_Pool *next_pool;
} Arena_Pool;


// can be done concurrently
Arena *Pool_Get(Arena_Pool *pool);
// can be done concurrently, (but two threads cannot release the same pool)
void Pool_Release(Arena_Pool *pool, Arena *to_release);

// this could do something fuck-y if done when multiple threads are running,
// but your an idiot to do that.
//
// also releases all pools.
void Pool_Free_Arenas(Arena_Pool *pool);



// ===================================================
//                    Dynamic Array
// ===================================================

#ifndef ARRAY_INITAL_CAPACITY
    #define ARRAY_INITAL_CAPACITY       32
#endif


//
// Example:
//   - make a variable
//      Array(Foo) foo_array;
//
//   - make a type
//      typedef Array(Bar) Bar_Array;
//
//   foo_array.items     = /* the array pointer */
//   foo_array.count     = /* number of items in array */
//   foo_array.capacity  = /* the capacity */
//   foo_array.allocator = /* a settable arena allocator */
//
#define Array(Type)                         \
    struct {                                \
        Type *items;                        \
        u64 count;                          \
        u64 capacity;                       \
        Arena *allocator;                   \
    }


// this struct shares the same shape as every array, so we can pass a pointer
// to an array into a function, and still work normally with its contents.
typedef struct {
    void *items;
    u64 count;
    u64 capacity;
    Arena *allocator;
} Generic_Array;

// so generic functions know how to manipulate the item pointer.
typedef struct {
    u64 item_size;
    u64 item_align;
} Array_Item_Type_Properties_Struct;


#define Array_Item_Size(a)      sizeof(*(a)->items)
#define Array_Item_Align(a)     Alignof(*(a)->items)
#define Get_Item_Type_Properties(array) ( (Array_Item_Type_Properties_Struct){ Array_Item_Size(array), Array_Item_Align(array) } )

// might increase the capacity of the array,
// array will be able to hold at least count elements.
void Array_Maybe_Grow(Generic_Array *array, Array_Item_Type_Properties_Struct item_properties, u64 new_count, bool clear_to_zero, Source_Code_Location caller_location);

// shifts the array left, why do i have this function?
void Array_Shift(Generic_Array *array, Array_Item_Type_Properties_Struct item_properties, u64 from_index);



// add a single value
//
// could be a function, but would have to take a void* and those suck.
// this is a macro so you dont have to make a reference every time you add something.
#define Array_Append(array, value)                                                                                                          \
    (Array_Maybe_Grow((Generic_Array*)(array), Get_Item_Type_Properties(array), (array)->count + 1, false, Get_Source_Code_Location()),     \
    (array)->items[(array)->count++] = (value))

#define Array_Add(array, n, zeroed)                                                                                                         \
    (Array_Maybe_Grow((Generic_Array*)(array), Get_Item_Type_Properties(array), (array)->count + (n), zeroed, Get_Source_Code_Location()),  \
    (array)->count += (n),                                                                                                                  \
    &(array)->items[(array)->count - (n)])



// make sure there is enough room to hold 'n' items, dose not increase count.
#define Array_Reserve(array, n)                                                                                 \
    (Array_Maybe_Grow((Generic_Array*)(array), Get_Item_Type_Properties(array), (n), false, Get_Source_Code_Location()))

#define Array_Insert(array, index, value)                                                                                                   \
    (Array_Maybe_Grow((Generic_Array*)(array), Get_Item_Type_Properties(array), (array)->count + 1, false, Get_Source_Code_Location()),     \
    Mem_Move((array)->items + (index), (array)->items + (index) - 1, ((array)->count - (index)) * Array_Item_Size(array)),                  \
    (array)->items[(index)] = (value),                                                                                                      \
    (array)->count += 1)


// TODO this could be a function.
#define Array_Remove(array, index, n)                                                                                                   \
    do {                                                                                                                                \
        ASSERT((0 <= (index) && (index) < (array)->count) && (0 <= (n) && (n) <= (array)->count - (index)));                            \
        Mem_Move((array)->items + (index), (array)->items + (index) + (n), ((array)->count - (index) - (n)) * Array_Item_Size(array));  \
        (array)->count -= (n);                                                                                                          \
    } while(0)

// Dose the full swap, so if you add +1 to the count, the item will return.
#define Array_Swap_And_Remove(array, index)                                 \
    do {                                                                    \
        ASSERT(0 <= (index) && (index) < (array)->count);                   \
        if ((index) != (array)->count-1) {                                  \
            Typeof(*(array)->items) tmp = (array)->items[(index)];          \
            (array)->items[(index)] = (array)->items[(array)->count-1];     \
            (array)->items[(array)->count-1] = tmp;                         \
        }                                                                   \
        (array)->count -= 1;                                                \
    } while (0)


// u64 index = it - array->items;
#define Array_For_Each(it, array)                                             \
    for (Typeof(*(array)->items) *it = (array)->items; it < (array)->items + (array)->count; it++)


#define Array_Free(array)                   \
    do {                                    \
        if ((array)->allocator != NULL) {   \
            fprintf(stderr, "=======================================================================================\n");       \
            fprintf(stderr, "Are you serious?\n");                                                                              \
            fprintf(stderr, "\n");                                                                                              \
            fprintf(stderr, "Did you just attempt to free an array that was allready given an allocator?\n");                   \
            fprintf(stderr, "Not just any allocator either, the only thing arrays accept are arena allocators.\n");             \
            fprintf(stderr, "\n");                                                                                              \
            fprintf(stderr, "You know I do this for you right?\n");                                                             \
            fprintf(stderr, "I give you all these tools and this is what you do with it?\n");                                   \
            fprintf(stderr, "Make a mistake that could have easily been ignored, look into this macro you just called.\n");     \
            fprintf(stderr, "I can check if you have an allocator and just ignore it, but I wont.\n");                          \
            fprintf(stderr, "\n");                                                                                              \
            fprintf(stderr, "I'm not gonna even ASSERT(false).\n");                                                             \
            fprintf(stderr, "I'm gonna let the segmentation falt, or the subtle memory bug do the talking for me.\n");          \
            fprintf(stderr, "\n");                                                                                              \
            fprintf(stderr, "I hope you have a terrible day.\n");                                                               \
            fprintf(stderr, "=======================================================================================\n");       \
        }                                   \
        BESTED_FREE((array)->items);        \
        (array)->items = NULL;              \
    } while (0)



// ===================================================
//                Dynamic Hash Map
// ===================================================

// how big the hashmap grows on the first insert.
#ifndef HASH_MAP_INITAL_CAPACITY
    #define HASH_MAP_INITAL_CAPACITY 32
#endif


// returns a hash of the key,
// size is the size of the key type.
typedef u64  (*Hash_Function    )(void *key, u64 size);
// checks equality between 2 keys. return true if equal.
//
// I know hash collisions are pretty unlikely, (if you use a
// good hash function), but if two key hash's collide, it could
// create the worst, most un-debuggable bug. you would spend
// every night in terror.
typedef bool (*Equality_Function)(void *key_a, void *key_b, u64 size);

//
// Example:
//   - make a variable:
//     Hash_Map(u32, f32) id_to_percent_map = ZEROED;
//
//   - make a type:
//     typedef Hash_Map(String, Baz) Baz_Hash_Map;
//
//   id_to_percent_map.count         = /* number of entries in hash map        */
//   id_to_percent_map.hash_function = /* hash function to use for the key     */
//   id_to_percent_map.eq_function   = /* equality function to use for the key */
//   id_to_percent_map.allocator     = /* a settable arena allocator           */
//   id_to_percent_map.default_value = /* the default value when you use Hash_Map_Get_Or_Default() and the key is not in the map */
//
// ```
//     Hash_Map(s32, String) hash_map = {
//         .hash_function = NULL, // the default hash function just takes
//         .eq_function   = NULL, // the bytes of your type and hash's it.
//
//         .default_value = S("NO VALUE"), // a default value
//     };
//
//    Hash_Map(String, s32) reverse_map = {
//         .hash_function = Hash_Map_Hash_String, // some hash functions are provided for the String types,
//         .eq_function   = Hash_Map_Eq_String,   // (as well as 'const char *' type, but who cares about that one.)
//
//         .default_value = 0, // by default this is allready zero.
//    };
// ```
//
#define Hash_Map(Key_Type, Value_Type)      \
    struct {                                \
        struct {                            \
            u64        hash;                \
            Key_Type   key;                 \
            Value_Type value;               \
        } *entries;                         \
                                            \
        /* total number of alive items in hash_map */   \
        u64 count;                          \
        /* total number dead items in hash map */       \
        u64 dead_count;                     \
        u64 capacity;                       \
                                            \
        Hash_Function     hash_function;    \
        Equality_Function eq_function;      \
                                            \
        /* Settable allocator */            \
        Arena *allocator;                   \
                                            \
        /* Default value of new items */    \
        Value_Type default_value;           \
    }



typedef struct {
    u64 hash;
    u8 key_and_value_data[];
} Generic_Entry;

typedef struct {
    void *entries;

    u64 count;
    u64 dead_count;
    u64 capacity;

    Hash_Function     hash_function;
    Equality_Function eq_function;

    Arena *allocator;

    // dont know how big this thing is. or where it is.
    u8 default_value_maybe[];
} Generic_Hash_Map;


// These could probably be u32's,
//
// but im betting on llvm or whatever to compile this whole
// thing into something better, also its not gonna cost much to
// pass this thing as an argument.
//
// also every instance of this struct is a constant,
// optimizers will have fun with that.
typedef struct {
    u64 key_size;
    u64 value_size;

    // used because predicting c struct packing
    // is not something I want to do right now.
    u64 entry_size;
    u64 entry_alignment;

    // key could be Aligned to 128-bit padding,
    // this is so I dont have to care about that.
    u64 key_offset_in_entry;
    u64 value_offset_in_entry;

    u64 default_value_offset_in_hash_map;

} Hash_Map_Key_Value_Type_Properties;

#define Get_Hash_Map_Type_Properties(hash_map)                                              \
    ((Hash_Map_Key_Value_Type_Properties) {                                                 \
        .key_size   = sizeof((hash_map)->entries->key),                                     \
        .value_size = sizeof((hash_map)->entries->value),                                   \
                                                                                            \
        .entry_size      = sizeof (*(hash_map)->entries),                                   \
        .entry_alignment = Alignof(*(hash_map)->entries),                                   \
                                                                                            \
        .key_offset_in_entry   = offsetof(Typeof(*(hash_map)->entries), key),               \
        .value_offset_in_entry = offsetof(Typeof(*(hash_map)->entries), value),             \
                                                                                            \
        .default_value_offset_in_hash_map = offsetof(Typeof(*(hash_map)), default_value),   \
    })


// Returns a pointer to the value at with that key,
//
// may return null if key dose not exist.
//
// this function, (as well as many other hash_map functions),
// rely on GNU compound statements, to provide the user the
// ability to hand this function both:
//     - a variable,
//     - and a constant value.
// you might have to turn some warnings off though.
#define Hash_Map_Get(hash_map, the_key)                                 \
    ({                                                                  \
        Typeof((hash_map)->entries->key) key_on_stack = (the_key);      \
        (Typeof((hash_map)->entries->value)*) Generic_Hash_Map_Get((Generic_Hash_Map*)(hash_map), &key_on_stack, Get_Hash_Map_Type_Properties(hash_map));   \
    })

// Returns a pointer to the value at with that key,
//
// creates a new default value item if the key is not in the map.
//
// always returns a valid pointer, will PANIC() otherwise.
//
// TODO what if the area the user gave us set "dont panic" mode?
#define Hash_Map_Get_Or_Default(hash_map, the_key)                      \
    ({                                                                  \
        Typeof((hash_map)->entries->key) key_on_stack = (the_key);      \
        (Typeof((hash_map)->entries->value)*) Generic_Hash_Map_Get_Or_Default((Generic_Hash_Map*)(hash_map), &key_on_stack, Get_Hash_Map_Type_Properties(hash_map), Get_Source_Code_Location());    \
    })

// the same as Hash_Map_Get_Or_Default() except it dose not set the default value.
#define Hash_Map_Put(hash_map, the_key)                                 \
    ({                                                                  \
        Typeof((hash_map)->entries->key) key_on_stack = (the_key);      \
        (Typeof((hash_map)->entries->value)*) Generic_Hash_Map_Put((Generic_Hash_Map*)(hash_map), &key_on_stack, Get_Hash_Map_Type_Properties(hash_map), Get_Source_Code_Location());   \
    })


// true if key is in map. returns bool type.
#define Hash_Map_Contains(hash_map, the_key)                            \
    ({                                                                  \
        Typeof((hash_map)->entries->key) key_on_stack = (the_key);      \
        Generic_Hash_Map_Contains((Generic_Hash_Map*)(hash_map), &key_on_stack, Get_Hash_Map_Type_Properties(hash_map));    \
    })


// remove a key and value from hash map,
//
// returns weather or not the key was in the hash map.
#define Hash_Map_Remove(hash_map, the_key)                              \
    ({                                                                  \
        Typeof((hash_map)->entries->key) key_on_stack = (the_key);      \
        Generic_Hash_Map_Remove((Generic_Hash_Map*)(hash_map), &key_on_stack, Get_Hash_Map_Type_Properties(hash_map));  \
    })

// remove an entry by pointer to the value,
//
// will ASSERT() that the value pointer is valid
// (is one of this hash maps entries)
//
// returns true if it was successful.
#define Hash_Map_Remove_By_Value(hash_map, value_ptr)                   \
    Generic_Hash_Map_Remove_By_Value((Generic_Hash_Map*)(hash_map), value_ptr, Get_Hash_Map_Type_Properties(hash_map))


// reserve space for num_to_reserver *TOTAL* items,
//
// may PANIC() if your computer runs out of memory.
#define Hash_Map_Reserve(hash_map, num_to_reserve)                      \
    Generic_Hash_Map_Reserve((Generic_Hash_Map*)(hash_map), (num_to_reserve), Get_Hash_Map_Type_Properties(hash_map), Get_Source_Code_Location())

// clear the hash map, keep the memory.
#define Hash_Map_Clear(hash_map)    Generic_Hash_Map_Clear((Generic_Hash_Map*)(hash_map), Get_Hash_Map_Type_Properties(hash_map))
// free the memory used, only use if you haven't set an allocator.
//
// only free()'s the entries pointer,
// you have to manage the other memory yourself.
#define Hash_Map_Free(hash_map)     Generic_Hash_Map_Free((Generic_Hash_Map*)(hash_map))


// return a pointer to the key for the value, please dont modify this.
//
// i could of just dereferenced this key, but
// it would be pretty easy to get around it.
// if what you were storing was an array or something.
#define Hash_Map_Key_For(hash_map, value_ptr)       \
    ((Typeof((hash_map)->entries->key)*) Generic_Hash_Map_Key_For((Generic_Hash_Map*)(hash_map), (value_ptr), Get_Hash_Map_Type_Properties(hash_map)))


// allows you to loop over all values in the hash table.
//
// ```
// Hash_Map(s32, u32) hash_map = ZEROED;
//
// *Hash_Map_Get_Or_Default(&hash_map, 52) = 6;
// *Hash_Map_Get_Or_Default(&hash_map, 12) = 8;
//
// Hash_Map_For_Each(value, &hash_map) {
//     s32 *key = Hash_Map_Key_For(&hash_map, value);
//
//     printf("%d => %u", *key, *value);
//
//     // you can also modify the value here.
//     *value += 1;
// }
// ```
#define Hash_Map_For_Each(value_it, hash_map)                       \
    for (                                                           \
        Typeof((hash_map)->entries->value) *value_it = NULL;        \
        Generic_Hash_Map_For_Each_Iterator_Next((Generic_Hash_Map*)(hash_map), (void**)&value_it, Get_Hash_Map_Type_Properties(hash_map));     \
    )



// this is the default for a hash table, if no hash function is set.
//
// hash's the data for the key.
//
// uses FNV-1a
u64  Hash_Map_Default_Hash_Function    (void *key, u64 size);
bool Hash_Map_Default_Equality_Function(void *key_a, void *key_b, u64 size);


// use these if your key type is String,
// hash entire string with Hash_Map_Default_Hash_Function()
u64  Hash_Map_Hash_String(void *key, u64 size);
bool Hash_Map_Eq_String  (void *key_a, void *key_b, u64 size);

// use this if your key type is a c String,
// hash entire string with Hash_Map_Default_Hash_Function()
u64  Hash_Map_Hash_C_String(void *key, u64 size);
bool Hash_Map_Eq_C_String  (void *key_a, void *key_b, u64 size);


// TODO maybe more hash functions,
//
// but really, who cares? fnv1a is good enough for every job,
// some might even say its a great hash function, that is more
// than fast enough for literally every job, sure you might like
// djb2 because its simpler or something, but any remotely
// good hash will do the exact same job.
u64 Hash_Function_fnv1a(void *key, u64 size);
// Hash_Function Hash_Function_fnv1(void *key, u64 size);


// The generic functions that power the hash map interface.

void *Generic_Hash_Map_Get                  (Generic_Hash_Map *hash_map, void *key,            Hash_Map_Key_Value_Type_Properties properties);
void *Generic_Hash_Map_Get_Or_Default       (Generic_Hash_Map *hash_map, void *key,            Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location);
void *Generic_Hash_Map_Put                  (Generic_Hash_Map *hash_map, void *key,            Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location);

bool Generic_Hash_Map_Contains              (Generic_Hash_Map *hash_map, void *key,            Hash_Map_Key_Value_Type_Properties properties);

void Generic_Hash_Map_Clear                 (Generic_Hash_Map *hash_map,                       Hash_Map_Key_Value_Type_Properties properties);
void Generic_Hash_Map_Free                  (Generic_Hash_Map *hash_map);
void Generic_Hash_Map_Reserve               (Generic_Hash_Map *hash_map, u64 num_to_reserve,   Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location);

bool Generic_Hash_Map_Remove                (Generic_Hash_Map *hash_map, void *key,            Hash_Map_Key_Value_Type_Properties properties);
bool Generic_Hash_Map_Remove_By_Value       (Generic_Hash_Map *hash_map, void *value_ptr,      Hash_Map_Key_Value_Type_Properties properties);

void *Generic_Hash_Map_Key_For              (Generic_Hash_Map *hash_map, void *value_ptr,      Hash_Map_Key_Value_Type_Properties properties);
bool Generic_Hash_Map_For_Each_Iterator_Next(Generic_Hash_Map *hash_map, void **current_value, Hash_Map_Key_Value_Type_Properties properties);



// ===================================================
//                       String
// ===================================================

// returns true if c is one of the ASCII whitespace characters.
bool Is_Whitespace(char c);
#define To_Upper(c) (('a' <= (c) && (c) <= 'z') ? ((c) - 'a' + 'A') : (c))
#define To_Lower(c) (('A' <= (c) && (c) <= 'Z') ? ((c) - 'A' + 'a') : (c))


typedef struct {
    char *data;
    u64 length;
} String;

typedef Array(String) String_Array;


// Formatting for printf()
#define S_Fmt    "%.*s"
#define S_Arg(s) (int) (s).length, (s).data

String  String_From_C_Str(const char *str);
#define S(c_str)    String_From_C_Str(c_str)


// will use BESTED_MALLOC() if allocator is NULL
const char *String_To_C_Str(String s, Arena *allocator);
const char *temp_String_To_C_Str(String s);


typedef struct {
    // the allocator to use, will use BESTED_MALLOC() if null.
    Arena* allocator;
    // weather or not to null terminate the string.
    bool null_terminate;
} String_Duplicate_Opt;

// if arena == NULL, uses BESTED_MALLOC.
//
// by default dose not null_terminate
#define String_Duplicate(string, ...) _String_Duplicate((string), (String_Duplicate_Opt){ __VA_ARGS__ })
String _String_Duplicate(String s, String_Duplicate_Opt opt);

// in place.
void    String_To_Upper(String *s);

bool    String_Eq           (String s1, String s2);
bool    String_Starts_With  (String s, String prefix);
bool    String_Ends_With    (String s, String postfix);

bool    String_Contains_Char        (String s, char c);
// finds the first occurrence of c in s, -1 on failure
s64     String_Find_Index_Of_Char   (String s, char c);
// finds the first occurrence of needle in s, -1 on failure
// if needle.size == 0, returns -1.
s64     String_Find_Index_Of        (String s, String needle);

// finds the line at index, and returns a String of the line, strips '\n'
String  String_get_single_line(String s, u64 index);

// advance the data, subtract the size, in place.
// dose not check for null, or do any bounds checking. thats on you.
void    String_Advance  (String *s, s64 count);
// advance the data, subtract the size, returns a new copy.
// dose not check for null, or do any bounds checking. thats on you.
String  String_Advanced (String s, s64 count);

typedef bool (*char_to_bool_func)(char);
// remove ASCII whitespace characters from the right of the String.
String  String_Trim_Right(String s);
// returns a String with the front chopped off, according to the test function.
// use Is_Whitespace to chop the whitespace off of the front.
String  String_Chop_While(String s, char_to_bool_func test_char_function);


typedef struct {
    // left side of the split, may be entire
    // string if separator was not present.
    String left;
    // right side of the split. may be empty.
    String right;
    // weather the separator was in the string at all.
    bool ok;
} Split_Once_Result;

// splits the string once.
//
// if the separator is not in the string, the left side
// returns the whole string, and the right string is empty.
Split_Once_Result Split_Once(String string, String separator);

// appends the string split by separator parts into the result array.
//
// returns the number of sections.
u64 String_Split_By(String string, String separator, String_Array *result);


String String_Path_to_Filename(String s);
String String_Remove_Extention(String s);

typedef enum {
    SGNL_Remove_Comments = 1 << 0,
    SGNL_Trim            = 1 << 1,
    SGNL_Skip_Empty      = 1 << 2,
    SGNL_All             = SGNL_Remove_Comments | SGNL_Trim | SGNL_Skip_Empty,
} String_Get_Next_Line_Flag;

// gets the next line in parseing, and advances parseing and line_num to match
// bool's toggle removeing comments that start with '#',
// trimming the output (from the right), and skiping over empty lines.
//
// 'result == parseing' signifies the end of the file.
// OR result.size == 0 if skip_empty is toggled.
String  String_Get_Next_Line(String *parseing, u64 *line_num, String_Get_Next_Line_Flag flags);


const char *temp_sprintf       (const char *format, ...) __attribute__ ((format (printf, 1, 2)));
// is null terminated
String      temp_String_sprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));



// ===================================================
//                    String Builder
// ===================================================

#define STRING_BUILDER_NUM_BUFFERS      32
static_assert(Is_Pow_2(STRING_BUILDER_NUM_BUFFERS), "For Speed");

// how much to alloc by default when a new segment of memory is needed.
#ifndef STRING_BUILDER_BUFFER_DEFAULT_SIZE
    // 4096 bytes is equal to the default page size in windows and linux. probably.
    #define STRING_BUILDER_BUFFER_DEFAULT_SIZE      (4 * KILOBYTE)
#endif

// This macro is called when something goes wrong. Recommended to just use assert.
//
// If the macro dose nothing, (or doesn't exist,) the results of some operations
// will return NULL/base values. or do no work at all. use at your own risk.
#ifndef STRING_BUILDER_PANIC
    #define STRING_BUILDER_PANIC(error_text)        ASSERT(false && error_text)
#endif


// String Builder internal structure
// holds the segments of the being built string.
typedef struct {
    char *data;
    u64 count;
    u64 capacity;
} Character_Buffer;

// String Builder internal structure
// Holds a bunch character buffers, and a pointer to the next Segment as well,
// Acts like a linked list node.
typedef struct Segment {
    struct Segment *next;
    Character_Buffer buffers[STRING_BUILDER_NUM_BUFFERS];
} Segment;

// A data-structure that can efficiently grow a string.
// good for medium to large strings. small strings (< 4096 bytes) are not recommended, as this structure will allocate once.
typedef struct String_Builder {
    // how much to allocate when a new segment is needed.
    u64 base_new_allocation;

    // the current segment were working on.
    // if its pointing to NULL, (aka you just zero initalized it), it will be set to a pointer to 'first_segment_holder'
    Segment *current_segment;

    // last buffer in use.
    u64 buffer_index;
    // the first in a linked list.
    Segment first_segment_holder;

    // set an allocator to use it to allocate, otherwise uses 'BESTED_MALLOC()'.
    Arena *allocator;
} String_Builder;


// current size of the string being built.
//
// this calculation runs though all of the segments, so dont spam it maybe.
u64 String_Builder_Count(String_Builder *sb);
// how much space the builder has.
// NOTE. some space will be wasted, for performance reasons.
u64 String_Builder_Total_Capacity(String_Builder *sb);

void String_Builder_Clear(String_Builder *sb);

// Dose not check for NULL byte
void String_Builder_Ptr_And_Size(String_Builder *sb, void *ptr, u64 size);
// Dose not check for NULL byte
void String_Builder_String(String_Builder *sb, String s);

u64 String_Builder_printf(String_Builder *sb, const char *format, ...) __attribute__ ((format (printf, 2, 3)));

#define String_Builder_Struct_Bytes(sb, struct_ptr) String_Builder_Ptr_And_Size((sb), (void*) (struct_ptr), sizeof(*(struct_ptr)))
#define String_Builder_Array_Bytes(sb, ptr, count)  String_Builder_Ptr_And_Size((sb), (void*)(ptr), sizeof(*(ptr)) * (count))


// A trailing NULL byte is appended, so you can pass it into functions that expect a C String
String String_Builder_To_String(String_Builder *sb);

// Dose not write a trailing NULL byte, (as apposed to SB_to_SV)
void String_Builder_To_File(String_Builder *sb, FILE *file);

// only call if your not useing an allocator.
void String_Builder_Free(String_Builder *sb);



// ===================================================
//                      Misc
// ===================================================

// these are just useful, dont have to make these every single project.
typedef Array(s64)    Int_Array;


// will return malloc'd string if arena is NULL,
String Read_Entire_File(String filename, Arena *arena);

// only works on unix.
u64 nanoseconds_since_unspecified_epoch(void);

bool check_if_file_exists(const char *filepath);



// ===================================================
//                 Debug Helpers
// ===================================================

// extremely annoyed that we have to pass pointers in here.
//
// _Generic() is just terrible.

const char *print_s64   (void *_x);
const char *print_u64   (void *_x);

const char *print_s32   (void *_x);
const char *print_u32   (void *_x);

const char *print_s16   (void *_x);
const char *print_u16   (void *_x);

const char *print_s8    (void *_x);
const char *print_u8    (void *_x);

const char *print_f32   (void *_x);
const char *print_f64   (void *_x);

const char *print_bool  (void *_x);
const char *print_string(void *_x);




#define generic_to_str(x)                                                       \
    _Generic(x,                                                                 \
        s64: print_s64(&x),  u64: print_u64(&x),                                \
        s32: print_s32(&x),  u32: print_u32(&x),                                \
        s16: print_s16(&x),  u16: print_u16(&x),                                \
        s8 : print_s8 (&x),  u8 : print_u8 (&x),                                \
        f32: print_f32(&x),  f64: print_f64(&x),                                \
        bool: print_bool(&x),                                                   \
        String: print_string(&x),                                               \
        char *: x, /* were just gonna assume this is null terminated... */      \
        default: "?UNKNOWN_TYPE?"                                               \
    )


#define debug(x)  do { Typeof(x) _x = (x); printf("DEBUG: %s = %s\n", #x, generic_to_str(_x)); } while (0)

#define debug_break() asm("int3")






#endif // BESTED_H





#ifdef BESTED_IMPLEMENTATION


#include <string.h>
#include <stdarg.h>



// ===================================================
//               Source Code Location
// ===================================================

bool source_code_location_eq(Source_Code_Location a, Source_Code_Location b) {
    if (a.line != b.line) return false;
    // this should work, the pointer would be the same.
    if (a.file != b.file) return false;
    // strcmp(a.file, b.file) == 0;
    return true;
}



// ===================================================
//        Useful Memory Manipulation Functions
// ===================================================

bool Mem_Is_Aligned(void *ptr, u64 alignment) {
    static_assert(sizeof(u64) >= sizeof(void*), "doing math with pointers");
    return ((u64)ptr) % alignment == 0;
}

u64 Mem_Align_Forward(u64 size, u64 alignment) {
    return Div_Ceil(size, alignment)  * alignment;
}
u64 Mem_Align_Back(u64 size, u64 alignment) {
    return Div_Floor(size, alignment) * alignment;
}

void *Mem_Byte_Offset(void *ptr, s64 bytes) {
    u8 *ptr_u8 = (u8*)ptr;
    return (void*)(ptr_u8 + bytes);
}

s64 Mem_Ptr_Diff(void *ptr1, void *ptr2) {
    static_assert(sizeof(u64) >= sizeof(void*), "I HATE 128-Bit pointers");
    u64 ptr1_u64 = Ptr_To_U64(ptr1);
    u64 ptr2_u64 = Ptr_To_U64(ptr2);

    if (ptr1_u64 >= ptr2_u64) {
        return (s64) (ptr1_u64 - ptr2_u64);
    } else {
        u64 result = ptr2_u64 - ptr1_u64;
        return -((s64) result);
    }
}


void *Mem_Copy(void *dest, void *src, u64 size) {
    return memcpy(dest, src, size);
}

void *Mem_Move(void *dest, void *src, u64 size) {
    return memmove(dest, src, size);
}

void *Mem_Set (void *ptr, u8 value, u64 size) {
    return memset(ptr, value, size);
}

s32 Mem_Cmp (void *ptr1, void *ptr2, u64 count) {
    return memcmp(ptr1, ptr2, count);
}



// ===================================================
//                      Arena
// ===================================================

internal Region *Arena_Internal_New_Region(u64 capacity_in_bytes) {
    Region *new_region = (Region*) BESTED_MALLOC(sizeof(Region) + capacity_in_bytes);
    if (new_region) {
        new_region->next                = NULL;
        new_region->count_in_bytes      = 0;
        new_region->capacity_in_bytes   = capacity_in_bytes;
        new_region->do_not_free_this    = false;
    }
    return new_region;
}

internal void Arena_Internal_Free_Region(Region *region) {
    if (!region->do_not_free_this) BESTED_FREE(region);
}

// inline because there is really nothing in this function.
// just some funny casts, and a call to Mem_Set()
internal inline void *Arena_Internal_Get_New_Memory_At_Last_Region(Arena *arena, u64 size_in_bytes, u64 alignment, bool clear_to_zero) {
    u64 aligned_ptr_u64 = Mem_Align_Forward(Ptr_To_U64(arena->last->data + arena->last->count_in_bytes), alignment);
    // u64 aligned_ptr_u64 = Ptr_To_U64(arena->last->data + arena->last->count_in_bytes);

    s64 how_far_forward = Mem_Ptr_Diff(U64_To_Ptr(aligned_ptr_u64), arena->last->data + arena->last->count_in_bytes);
    ASSERT(how_far_forward >= 0);

    // lets hope this doesn't happen.
    if (arena->last->count_in_bytes + how_far_forward + size_in_bytes > arena->last->capacity_in_bytes) {
        return NULL;
    }


    if (clear_to_zero) Mem_Zero(U64_To_Ptr(aligned_ptr_u64), size_in_bytes);

    arena->last->count_in_bytes += size_in_bytes + (u64) how_far_forward;
    ASSERT(arena->last->count_in_bytes <= arena->last->capacity_in_bytes);
    return U64_To_Ptr(aligned_ptr_u64);
}


void *_Arena_Alloc(Arena *arena, u64 size_in_bytes, Arena_Alloc_Opt opt, Source_Code_Location caller_location) {
    // TODO track allocations.

    u64 default_size = (arena->minimum_allocation_size != 0) ? arena->minimum_allocation_size : ARENA_REGION_DEFAULT_CAPACITY;

    // add the extra alignment because if you have to allocate the thing,
    // the alignment of the Region is kinda random, with + alignment, the allocation will always succeed.
    u64 to_alloc_if_no_room = Max(default_size, size_in_bytes + opt.alignment);

    // if the arena currently holds no memory
    if (arena->last == NULL) {
        if (arena->first != NULL) {
            ARENA_PANIC(caller_location, "arena->first != NULL, only these library functions should touch the insides of an arena.");
            return NULL;
        }

        if (arena->panic_when_trying_to_allocate_new_page) {
            ARENA_PANIC(caller_location, "Arena_alloc: attempted to allocate new memory, but that has been disallowed. (when there was no memory to begin with.)");
            return NULL;
        }

        arena->last = Arena_Internal_New_Region(to_alloc_if_no_room);
        if (arena->last == NULL) {
            if (arena->dont_panic_when_allocation_failure) return NULL;
            ARENA_PANIC(caller_location, "Arena_alloc: attempted to allocate new memory, got null. (when there was no memory to begin with.)");
            return NULL;
        }

        arena->first = arena->last;

        void *new_memory = Arena_Internal_Get_New_Memory_At_Last_Region(arena, size_in_bytes, opt.alignment, opt.clear_to_zero);
        if (!new_memory) {
            ARENA_PANIC(caller_location, "new_memory from internal allocator returned null wtf.");
        }
        return new_memory;
    }

    // find room, or find the end
    while ((arena->last->count_in_bytes + size_in_bytes + opt.alignment > arena->last->capacity_in_bytes) && (arena->last->next != NULL)) {
        arena->last = arena->last->next;
        if (arena->last) {
            // if we just discoverd this, it must be zero'd.
            // this helps with our mark implementation as well.
            arena->last->count_in_bytes = 0;
        }
    }

    if (arena->last->count_in_bytes + size_in_bytes + opt.alignment <= arena->last->capacity_in_bytes) {
        // if there is space alloc
        void *new_memory = Arena_Internal_Get_New_Memory_At_Last_Region(arena, size_in_bytes, opt.alignment, opt.clear_to_zero);
        if (!new_memory) {
            ARENA_PANIC(caller_location, "new_memory from internal allocator returned null wtf.");
        }
        return new_memory;

    } else {
        // we need a new region

        if (arena->last->next != NULL) {
            ARENA_PANIC(caller_location, "arena->last->next != NULL, only these library functions should touch the insides of an arena, and they should never produce this state.");
            return NULL;
        }

        if (arena->panic_when_trying_to_allocate_new_page) {
            ARENA_PANIC(caller_location, "Arena_alloc: attempted to allocate new memory, but that has been disallowed.");
            return NULL;
        }

        Region *last_last = arena->last;

        arena->last = Arena_Internal_New_Region(to_alloc_if_no_room);
        if (arena->last == NULL) {
            if (arena->dont_panic_when_allocation_failure) return NULL;
            ARENA_PANIC(caller_location, "Arena_alloc: attempted to allocate new memory, got null.");
        }
        last_last->next = arena->last;


        void *new_memory = Arena_Internal_Get_New_Memory_At_Last_Region(arena, size_in_bytes, opt.alignment, opt.clear_to_zero);
        if (!new_memory) {
            ARENA_PANIC(caller_location, "new_memory from internal allocator returned null wtf.");
        }
        return new_memory;
    }
}



Arena_Mark Arena_Get_Mark(Arena *arena) {
    Arena_Mark result = {
        .last = arena->last,
        .count = arena->last ? arena->last->count_in_bytes : 0,
    };
    return result;
}
void Arena_Set_To_Mark(Arena *arena, Arena_Mark mark) {
    arena->last = mark.last ? mark.last : arena->first;
    if (arena->last) {
        arena->last->count_in_bytes = mark.count;
    }
}


void _Arena_Initialize_First_Page(Arena *arena, u64 first_page_size_in_bytes, Source_Code_Location caller_location) {
    if (arena->first != NULL) return;

    u64 tmp_min_alloc_size = arena->minimum_allocation_size;
    arena->minimum_allocation_size = first_page_size_in_bytes;

    _Arena_Alloc(
        arena, 0,
        (Arena_Alloc_Opt){.alignment = Default_Alignment, .clear_to_zero = false, },
        caller_location
    );

    arena->minimum_allocation_size = tmp_min_alloc_size;
}


void _Arena_Add_Buffer_As_Storage_Space(Arena *arena, void *buffer, u64 buffer_size_in_bytes, Source_Code_Location caller_location) {
    if (buffer == NULL) {
        ARENA_PANIC(caller_location, "Arena_Add_Buffer_As_Storeage_Space: buffer != NULL, why did you pass this to us.");
        return;
    }
    if (buffer_size_in_bytes <= sizeof(Region)) {
        ARENA_PANIC(caller_location, "Arena_Add_Buffer_As_Storeage_Space: buffer_size_in_bytes <= sizeof(Region), The passed in buffer must be big enough to contain the Region, preferably much bigger");
        return;
    }

    u64 real_allocatable_space = buffer_size_in_bytes - sizeof(Region);

    Region *new_region = (Region*) buffer;
    new_region->count_in_bytes      = 0;
    new_region->capacity_in_bytes   = real_allocatable_space;
    new_region->next                = NULL;
    new_region->do_not_free_this    = true;


    if (arena->last == NULL) {
        if (arena->first != NULL) {
            ARENA_PANIC(caller_location, "Arena_Add_Buffer_As_Storeage_Space: arena->first != NULL, when arena->last == NULL, something went wrong internally");
        }
        arena->first = new_region;
        arena->last  = new_region;

    } else {
        Region *p;
        for (p = arena->last; p->next != NULL; p = p->next);
        p->next = new_region;
    }
}


const char *Arena_sprintf(Arena *arena, const char *format, ...) {
    char *buf    = NULL;
    u64 buf_size = 0;

    if (arena->last != NULL) {
        buf      = (char*) (arena->last->data + arena->last->count_in_bytes);
        buf_size = arena->last->capacity_in_bytes - arena->last->count_in_bytes;
    }

    va_list args;

    va_start(args, format);
        s64 formatted_size = vsnprintf(buf, buf_size, format, args);
    va_end(args);

    if (formatted_size < 0) {
        // TODO ? accept file and line here? i never use this function anyway...
        ARENA_PANIC(Get_Source_Code_Location(), "Arena_sprintf: format was not successful");
        return NULL;
    }

    // i dont know if this should be <= or just <, hmm...
    if ((u64)formatted_size < buf_size) {
        // the string fits!
        // advance the count. +1 because we need the null terminator for this one.
        arena->last->count_in_bytes += (u64)formatted_size+1;
        return buf;


    } else {
        // else we need to allocate some space.

        buf      = (char*)Arena_Alloc(arena, (u64)formatted_size+1, .clear_to_zero = false);
        // buf      = (char*)Arena_Alloc_Clear(arena, (u64)formatted_size+1, false);
        buf_size = (u64)formatted_size+1;

        // only happens when Arena_Alloc either returns null because it was
        // allowed to return null, or it panic'd (and the panic function was
        // replaced with something that doesn't abort), either way the user
        // of this function will expect this to maybe be null.
        if (!buf) return NULL;

        va_start(args, format);
            vsnprintf(buf, buf_size, format, args);
        va_end(args);

        return buf;
    }
}


void Arena_Clear(Arena *arena) {
    if (arena->first == NULL) return;

    for (Region *r = arena->first; r != arena->last; r = r->next) {
        r->count_in_bytes = 0;
    }
    arena->last->count_in_bytes = 0;

    arena->last = arena->first;
}

void Arena_Free (Arena *arena) {
    // this is slightly more complicated than it would be,
    // but you cant use the thing you just free'd, for some reason,
    // because it get's messed with by 'free'
    Region *r = arena->first;
    while (r) {
        Region *next = r->next;
        Arena_Internal_Free_Region(r);
        r = next;
    }

    arena->first = NULL;
    arena->last  = NULL;
}



// ===================================================
//                      Pool
// ===================================================

Arena *Pool_Get(Arena_Pool *pool) {
    while (true) {
        for (u32 i = 0; i < NUM_POOL_ARENAS; i++) {
            if (Has_Bit(Atomic_Load(&pool->in_use_flags), i)) continue;

            Pool_Flag_Type before = Atomic_Or(&pool->in_use_flags, Bit(i));
            if (Has_Bit(before, i)) continue; // someone got to it first,

            Arena *new_arena = &pool->arena_pool[i];
            Arena_Clear(new_arena);
            return new_arena;
        }

        Atomic(bool) *lock = &pool->creating_new_pool_in_chain_lock;
        Atomic_Capture_Lock(lock) {
            if (!pool->next_pool) {
                pool->next_pool = (Arena_Pool*)BESTED_MALLOC(sizeof(Arena_Pool));
                Mem_Zero(pool->next_pool, sizeof(Arena_Pool));
            }
            pool = pool->next_pool;
        }
    }

    UNREACHABLE();
}

void Pool_Release(Arena_Pool *pool, Arena *to_release) {
    while (pool) {
        s64 maybe_index = Mem_Ptr_Diff(to_release, pool->arena_pool);

        if (Is_Between(maybe_index, 0, (s64) ((NUM_POOL_ARENAS-1) * sizeof(Arena)))) {
            ASSERT(maybe_index % sizeof(Arena) == 0);
            s64 index = maybe_index / sizeof(Arena);

            ASSERT(Has_Bit( Atomic_Load(&pool->in_use_flags) , index));
            Atomic_Xor(&pool->in_use_flags, Bit(index)); // clear the flag atomically.
            return;
        }

        Atomic(bool) *lock = &pool->creating_new_pool_in_chain_lock;
        Atomic_Capture_Lock(lock) {
            pool = pool->next_pool;
        }
    }
    // Ran out of pools to check, to_release must not be an arena from this pool.
    UNREACHABLE();
}

void Pool_Free_Arenas(Arena_Pool *pool) {
    // the user just passed up the original pool.
    // dont free it, but free everything else

    Arena_Pool *original_pool = pool;

    // free all the arena's.
    while (pool) {
        for (u32 i = 0; i < NUM_POOL_ARENAS; i++) {
            Arena_Free(&pool->arena_pool[i]);
        }
        pool = pool->next_pool;
    }

    // free the pointers to the pools.
    pool = original_pool->next_pool;
    while (pool) {
        Arena_Pool *next_pool = pool->next_pool;
        BESTED_FREE(pool);
        pool = next_pool;
    }

    // clear this entire struct, make ready to use again,
    // it should be ready to use again
    //
    // removes settings from all arena's
    //
    // probably should change this behaviour if pool ever gets
    // any settings, like the arena has
    Mem_Zero_Struct(original_pool);
}



// ===================================================
//                Dynamic Arrays
// ===================================================

void Array_Maybe_Grow(Generic_Array *array, Array_Item_Type_Properties_Struct item_properties, u64 new_count, bool clear_to_zero, Source_Code_Location caller_location) {
    ASSERT(array); // would be kinda weird.

    // if the new count is less, we dont need to do anything. not even clear anything.
    if (new_count <= array->count) return;

    if (new_count > array->capacity) {
        array->capacity = array->capacity ? array->capacity * 2 : ARRAY_INITAL_CAPACITY;
        while (array->capacity < new_count) array->capacity *= 2;

        void *new_array = NULL;
        if (array->allocator) {

            // special case where the last thing to allocate was this current array,
            // in this case we dont need to do any Mem_Copy, just advance the count_in_bytes.
            if (array->allocator->last) {
                bool last_allocation_was_this_array = (array->allocator->last->data + array->allocator->last->count_in_bytes) - (array->count * item_properties.item_size) == array->items;
                if (last_allocation_was_this_array) {
                    u64 number_of_new_elements = array->capacity - array->count;
                    u64 new_amount_to_allocate = number_of_new_elements * item_properties.item_size;
                    bool new_array_can_fit_into_current_region = array->allocator->last->count_in_bytes + new_amount_to_allocate <= array->allocator->last->capacity_in_bytes;

                    if (new_array_can_fit_into_current_region) {
                        // TODO maybe call this function, this is some dangerous manipulation.
                        // Arena_Alloc(array->allocator, new_amount_to_allocate);
                        array->allocator->last->count_in_bytes += new_amount_to_allocate;
                        goto skip_allocation;
                    }
                }
            }

            // need to do this, need to set file and line properly.
            //
            // void *_Arena_Alloc(Arena *arena, u64 size_in_bytes, Arena_Alloc_Opt opt, const char *file, s32 line);
            new_array = _Arena_Alloc(
                array->allocator, item_properties.item_size * array->capacity,
                (Arena_Alloc_Opt){.alignment = item_properties.item_align, .clear_to_zero = false, },
                caller_location
            );
        } else {
            new_array = BESTED_ALIGNED_ALLOC(item_properties.item_align, item_properties.item_size * array->capacity);
        }

        // malloc may return null, or the arena can be set
        // to return NULL instead of panic'ing for an allocation.
        //
        // no nice way to handle this, and this situation
        // is so rare, I dont care about it.
        //
        // eat this panic.
        if (new_array == NULL) {
            PANIC(SCL_Fmt" got null when trying to grow array", SCL_Arg(caller_location));
        }

        Mem_Copy(new_array, array->items, item_properties.item_size * array->count);

        if (!array->allocator) BESTED_FREE(array->items);
        array->items = new_array;
    }

skip_allocation:
    if (clear_to_zero) {
        u64 new_to_add = new_count - array->count;
        Mem_Zero((u8*)array->items + (item_properties.item_size * array->count), item_properties.item_size * new_to_add);
    }
}


void Array_Shift(Generic_Array *array, Array_Item_Type_Properties_Struct item_properties, u64 from_index) {
    ASSERT(array); // would be kinda weird.

    Mem_Move(array->items, (u8*)array->items + from_index, (array->count - from_index) * item_properties.item_size);
    array->count -= from_index;
}



// ===================================================
//                Dynamic Hash Map
// ===================================================

// Hash Map helper functions / macros.

#define Hash_Map_UNALLOCATED    (0)
#define Hash_Map_DEAD           (1)

#define HASH_MAP_HASH_FROM_ENTRY(entry) (((Generic_Entry*)entry)->hash)

// why use a macro when you can just not?
internal bool Hash_Map_Hash_Is_Bad(u64 hash) {
    return (hash == Hash_Map_UNALLOCATED) || (hash == Hash_Map_DEAD);
}

// return NULL if there are no entries,
//
// else either return the real entry that holds that key,
//
// or return somewhere new to put the key
internal void *Hash_Map_Maybe_Get_Entry(Generic_Hash_Map *hash_map, void *key, u64 hash, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    // don't give this an invalid hash.
    ASSERT(!Hash_Map_Hash_Is_Bad(hash));

    if (hash_map->capacity == 0) return NULL;

    // gonna need this to check if keys are equal.
    Equality_Function equality_function = hash_map->eq_function ? hash_map->eq_function : Hash_Map_Default_Equality_Function;

    // must be true, or my probe strategy will not cover every cell.
    ASSERT(Is_Pow_2(hash_map->capacity));
    u64 increment   = 1;
    u64 entry_index = hash % hash_map->capacity;

    while (true) {
        void *entry = (u8*)hash_map->entries + entry_index * properties.entry_size;

        // this is a valid position to put something in. break
        if (HASH_MAP_HASH_FROM_ENTRY(entry) == Hash_Map_UNALLOCATED) break;

        // check if this is the same key.
        if (HASH_MAP_HASH_FROM_ENTRY(entry) == hash && equality_function(key, (u8*)entry + properties.key_offset_in_entry, properties.key_size)) {
            return entry;
        }

        entry_index = (entry_index + increment) % hash_map->capacity;
        increment += 1;
        ASSERT(increment < 4096); // what are the odds for 4096 hash collisions in a row? something bad must have happened.
    }

    // TODO maybe better to return a DEAD position? we can keep track if we have seen one.
    return (u8*)hash_map->entries + entry_index * properties.entry_size;
}

// returns the entry that uses this key,
//
// if the key dose not exist in the hash map, return NULL
internal void *Hash_Map_Get_Entry_Of_Key(Generic_Hash_Map *hash_map, void *key, u64 hash, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    void *entry = Hash_Map_Maybe_Get_Entry(hash_map, key, hash, properties);

    if (entry == NULL) return NULL;

    u64 entry_hash = HASH_MAP_HASH_FROM_ENTRY(entry);
    if (Hash_Map_Hash_Is_Bad(entry_hash)) return NULL;

    ASSERT(entry_hash == hash);
    return entry;
}

// hash's that are equal to Hash_Map_UNALLOCATED or Hash_Map_DEAD are not good.
internal u64 Hash_Map_Safely_Get_Hash(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    Hash_Function hash_function = hash_map->hash_function ? hash_map->hash_function : Hash_Map_Default_Hash_Function;
    u64 key_hash = hash_function(key, properties.key_size);

    if (Hash_Map_Hash_Is_Bad(key_hash)) {
        // some random number, to place the key randomly in the resulting hashmap,
        //
        // (this number is just the FNV 64-bit offset)
        return 14695981039346656037ULL;
    }

    return key_hash;
}

internal void *Generic_Hash_Map_Get_Entry_For_Value(Generic_Hash_Map *hash_map, void *value_ptr, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(value_ptr);

    void *entry = (u8*)value_ptr - properties.value_offset_in_entry;

    { // do some checks to be sure we got a valid pointer.
        // should map down into entry.
        s64 index_of_entry = ((u8*)entry - (u8*)hash_map->entries) / properties.entry_size;

        // make sure this is in the range of the hash map.
        ASSERT(Is_Between(index_of_entry, 0, (s64)hash_map->capacity-1));

        // make sure we got the right thing. make sure were not about to write somewhere stupid.
        ASSERT((u8*)hash_map->entries + index_of_entry * properties.entry_size + properties.value_offset_in_entry == value_ptr);
    }

    return entry;
}

internal void Hash_Map_Maybe_Grow(Generic_Hash_Map *hash_map, u64 be_able_to_fit_at_least, Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location) {
    ASSERT(hash_map);

    const u64 HASH_MAP_GROWTH_PERCENT = 75;

    // only grow array when at nearing capacity, not at capacity.
    if (be_able_to_fit_at_least < hash_map->capacity * HASH_MAP_GROWTH_PERCENT / 100) return;

    void *old_entries = hash_map->entries;
    u64   old_size    = hash_map->capacity;
    u64   old_count = hash_map->count;

    if (old_size == 0) ASSERT(old_entries == NULL);
    else               ASSERT(old_entries != NULL);

    // grow array capacity.
    hash_map->capacity = hash_map->capacity != 0 ? hash_map->capacity * 2 : HASH_MAP_INITAL_CAPACITY;
    while (be_able_to_fit_at_least >= hash_map->capacity * HASH_MAP_GROWTH_PERCENT / 100) {
        hash_map->capacity *= 2;
    }

    // get the new memory.
    if (hash_map->allocator) {
        // have to do this to set the caller location correctly.
        hash_map->entries = _Arena_Alloc(
            hash_map->allocator, properties.entry_size * hash_map->capacity,
            (Arena_Alloc_Opt){ .alignment = properties.entry_alignment, .clear_to_zero = true, },
            caller_location
        );
    } else {
        hash_map->entries = BESTED_ALIGNED_ALLOC(properties.entry_alignment, hash_map->capacity * properties.entry_size);
        // remember to clear malloc memory.
        Mem_Zero(hash_map->entries, hash_map->capacity * properties.entry_size);
    }

    // arena's can be set so they dont panic when
    // getting a NULL pointer from malloc,
    //
    // there is no neat way to say this function failed,
    // so im just gonna panic here.
    //
    // if your in a situation where memory might run out,
    // your machine is probably so low powered, a hash map
    // wouldn't have that many items in it anyway.
    // use a raw Arena instead.
    if (hash_map->entries == NULL) {
        PANIC(SCL_Fmt" got null pointer when trying to allocate memory for arena growth.", SCL_Arg(caller_location));
    }

    // reset fields that need resetting.
    hash_map->count = 0;
    hash_map->dead_count = 0;

    // now copy over the old entries
    for (u64 i = 0; i < old_size; i++) {
        void *this_entry = (u8*)old_entries + i * properties.entry_size;
        u64   this_hash  = HASH_MAP_HASH_FROM_ENTRY(this_entry);

        // dont care about bad entries.
        if (Hash_Map_Hash_Is_Bad(this_hash)) continue;

        void *new_entry = Hash_Map_Maybe_Get_Entry(hash_map, (u8*)this_entry + properties.key_offset_in_entry, this_hash, properties);
        // just checking to see if we got a good place.
        ASSERT(new_entry != NULL);
        ASSERT(HASH_MAP_HASH_FROM_ENTRY(new_entry) == Hash_Map_UNALLOCATED);

        // copy the new entry in.
        Mem_Copy(new_entry, this_entry, properties.entry_size);

        // we just added a new item!
        hash_map->count += 1;
    }

    ASSERT(hash_map->count == old_count);

    if (!hash_map->allocator) {
        // remember to clear the old entries.
        //
        // its ok to free a null pointer.
        BESTED_FREE(old_entries);
    }
}



void *Generic_Hash_Map_Get(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    u64 key_hash = Hash_Map_Safely_Get_Hash(hash_map, key, properties);

    void *entry = Hash_Map_Get_Entry_Of_Key(hash_map, key, key_hash, properties);
    if (entry == NULL) return NULL;

    return (u8*)entry + properties.value_offset_in_entry;
}

internal void *Generic_Hash_Map_Put_Or_Get_Default_Helper(Generic_Hash_Map *hash_map, void *key, bool set_default, Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location) {
    ASSERT(hash_map);
    ASSERT(key);

    u64 key_hash = Hash_Map_Safely_Get_Hash(hash_map, key, properties);

    // This might make the hash map grow, even when in
    // some cases it shouldn't, do I care?
    Hash_Map_Maybe_Grow(hash_map, hash_map->dead_count + hash_map->count + 1, properties, caller_location);

    // must be space to put this new thing.
    ASSERT(hash_map->capacity > 0);

    void *entry = Hash_Map_Maybe_Get_Entry(hash_map, key, key_hash, properties);

    // we just grew the array, this must either be
    // the correct key, or something unallocated.
    ASSERT(entry != NULL);

    if (HASH_MAP_HASH_FROM_ENTRY(entry) == Hash_Map_UNALLOCATED) {
        // set hash.
        ((Generic_Entry*) entry)->hash = key_hash;
        // set key
        Mem_Copy((u8*)entry + properties.key_offset_in_entry, key, properties.key_size);
        if (set_default) {
            // set default value.
            Mem_Copy((u8*)entry + properties.value_offset_in_entry, (u8*)hash_map + properties.default_value_offset_in_hash_map, properties.value_size);
        }

        hash_map->count += 1;
    }

    return (u8*)entry + properties.value_offset_in_entry;
}

void *Generic_Hash_Map_Get_Or_Default(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location) {
    ASSERT(hash_map);
    ASSERT(key);

    return Generic_Hash_Map_Put_Or_Get_Default_Helper(hash_map, key, true, properties, caller_location);
}

void *Generic_Hash_Map_Put(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location) {
    ASSERT(hash_map);
    ASSERT(key);

    return Generic_Hash_Map_Put_Or_Get_Default_Helper(hash_map, key, false, properties, caller_location);
}


bool Generic_Hash_Map_Contains(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    u64 key_hash = Hash_Map_Safely_Get_Hash(hash_map, key, properties);
    void *entry = Hash_Map_Get_Entry_Of_Key(hash_map, key, key_hash, properties);
    return entry != NULL;
}

void Generic_Hash_Map_Clear(Generic_Hash_Map *hash_map, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);

    // set all entries to unallocated.
    for (u64 i = 0; i < hash_map->capacity; i++) {
        Generic_Entry *entry = (void*)((u8*)hash_map->entries + i * properties.entry_size);
        entry->hash = Hash_Map_UNALLOCATED;
    }

    hash_map->count = 0;
    hash_map->dead_count = 0;
}

void Generic_Hash_Map_Free(Generic_Hash_Map *hash_map) {
    ASSERT(hash_map);

    if (hash_map->allocator) {
        fprintf(stderr, "=======================================================================================\n");
        fprintf(stderr, "Are you serious?\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Did you just attempt to free a hash_map that was allready given an allocator?\n");
        fprintf(stderr, "Not just any allocator either, the only thing hash_maps accept are arena allocators.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "You know I do this for you right?\n");
        fprintf(stderr, "I give you all these tools and this is what you do with it?\n");
        fprintf(stderr, "Make a mistake that could have easily been ignored, look into this function you just called.\n");
        fprintf(stderr, "I can check if you have an allocator and just ignore it, but I wont.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "I'm not gonna even ASSERT(false).\n");
        fprintf(stderr, "I'm gonna let the segmentation falt, or the subtle memory bug do the talking for me.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "I hope you have a terrible day.\n");
        fprintf(stderr, "=======================================================================================\n");
    }
    BESTED_FREE(hash_map->entries);
    hash_map->entries = NULL;

    hash_map->count      = 0;
    hash_map->dead_count = 0;
    hash_map->capacity   = 0;
}

void Generic_Hash_Map_Reserve(Generic_Hash_Map *hash_map, u64 num_to_reserve, Hash_Map_Key_Value_Type_Properties properties, Source_Code_Location caller_location) {
    ASSERT(hash_map);

    Hash_Map_Maybe_Grow(hash_map, num_to_reserve, properties, caller_location);
}

bool Generic_Hash_Map_Remove(Generic_Hash_Map *hash_map, void *key, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(key);

    u64 key_hash = Hash_Map_Safely_Get_Hash(hash_map, key, properties);

    void *entry = Hash_Map_Get_Entry_Of_Key(hash_map, key, key_hash, properties);
    if (entry == NULL) return false;

    u64 hash = HASH_MAP_HASH_FROM_ENTRY(entry);
    if (hash == Hash_Map_UNALLOCATED) return false;
    if (hash == Hash_Map_DEAD)        return false;

    ASSERT(key_hash == hash);
    ((Generic_Entry*) entry)->hash = Hash_Map_DEAD;
    hash_map->count      -= 1;
    hash_map->dead_count += 1;
    return true;
}

bool Generic_Hash_Map_Remove_By_Value(Generic_Hash_Map *hash_map, void *value_ptr, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(value_ptr);

    void *entry = Generic_Hash_Map_Get_Entry_For_Value(hash_map, value_ptr, properties);

    // would be super weird if this was the case.
    u64 hash = HASH_MAP_HASH_FROM_ENTRY(entry);
    if (hash == Hash_Map_UNALLOCATED) return false;
    if (hash == Hash_Map_DEAD)        return false;

    ((Generic_Entry*) entry)->hash = Hash_Map_DEAD;
    hash_map->count      -= 1;
    hash_map->dead_count += 1;
    return true;
}


void *Generic_Hash_Map_Key_For(Generic_Hash_Map *hash_map, void *value_ptr, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(value_ptr);

    void *entry = Generic_Hash_Map_Get_Entry_For_Value(hash_map, value_ptr, properties);
    return (u8*)entry + properties.key_offset_in_entry;
}

// returns if we should continue runing
bool Generic_Hash_Map_For_Each_Iterator_Next(Generic_Hash_Map *hash_map, void **current_value, Hash_Map_Key_Value_Type_Properties properties) {
    ASSERT(hash_map);
    ASSERT(current_value);

    void *start = (u8*)hash_map->entries;
    void *end   = (u8*)hash_map->entries + hash_map->capacity * properties.entry_size;

    void *current_entry;

    if (*current_value == NULL) {
        // this is the start of the iteration.
        // return first element.
        current_entry = start;
    } else {
        current_entry = Generic_Hash_Map_Get_Entry_For_Value(hash_map, *current_value, properties);
        // also move 1 along.
        current_entry = (u8*)current_entry + 1 * properties.entry_size;
    }

    // while the entry is empty. continue.
    while (current_entry < end && Hash_Map_Hash_Is_Bad(HASH_MAP_HASH_FROM_ENTRY(current_entry))) {
        current_entry = (u8*)current_entry + 1 * properties.entry_size;
    }

    *current_value = (u8*)current_entry + properties.value_offset_in_entry;
    return (u8*)current_entry < (u8*)end;
}



u64 Hash_Map_Default_Hash_Function(void *key, u64 size) {
    ASSERT(key);

    return Hash_Function_fnv1a(key, size);
}
bool Hash_Map_Default_Equality_Function(void *key_a, void *key_b, u64 size) {
    ASSERT(key_a && key_b);

    return Mem_Eq(key_a, key_b, size);
}


u64 Hash_Map_Hash_String  (void *key, u64 size) {
    ASSERT(key);
    ASSERT(size == sizeof(String));

    String *string = key;
    return Hash_Map_Default_Hash_Function(string->data, string->length);
}
bool Hash_Map_Eq_String(void *key_a, void *key_b, u64 size) {
    ASSERT(key_a && key_b);
    ASSERT(size == sizeof(String));

    String *string_a = key_a;
    String *string_b = key_b;
    return String_Eq(*string_a, *string_b);
}


u64 Hash_Map_Hash_C_String  (void *key, u64 size) {
    ASSERT(key);
    ASSERT(size == sizeof(const char *));

    const char **c_str = key;
    // yeah I know, this passes over the string twice,
    //
    // I do not care.
    //
    // just use the String type to begin with.
    String string = S(*c_str);
    return Hash_Map_Default_Hash_Function(string.data, string.length);
}
bool Hash_Map_Eq_C_String(void *key_a, void *key_b, u64 size) {
    ASSERT(key_a && key_b);
    ASSERT(size == sizeof(const char *));

    const char **c_str_a = key_a;
    const char **c_str_b = key_b;
    // it would be so much better if we had a way
    // of telling what the size of the string was...
    String string_a = S(*c_str_a);
    String string_b = S(*c_str_b);
    return String_Eq(string_a, string_b);
}


u64 Hash_Function_fnv1a(void *key, u64 size) {
    // its ok for the key to be NULL here,
    // could be hashing a string or something
    if (size > 0) ASSERT(key);

    // 64 bit offset_basis = 14695981039346656037
    const u64 FNV_offset = 14695981039346656037ULL;
    // 64 bit FNV_prime = 2^40 + 2^8 + 0xb3 = 1099511628211
    const u64 FNV_prime  =        1099511628211ULL;

    u8 *u8_ptr = key;
    u64 hash = FNV_offset;
    for (u64 i = 0; i < size; i++) {
        hash = (hash ^ u8_ptr[i]) * FNV_prime;
    }
    return hash;
}



// ===================================================
//                       String
// ===================================================

bool Is_Whitespace(char c) {
    if (c == ' ' ) return true;
    if (c == '\f') return true;
    if (c == '\n') return true;
    if (c == '\r') return true;
    if (c == '\t') return true;
    if (c == '\v') return true;
    return false;
}


String String_From_C_Str(const char *str) {
    String result = {
        .data   = (char *) str,
        .length = strlen(str),
    };
    return result;
}
const char *String_To_C_Str(String s, Arena *allocator) {
    return String_Duplicate(s, .allocator = allocator, .null_terminate = true).data;
}

#define TEMP_STRING_TO_C_STR_NUM_BUFFERS    64
#define TEMP_STRING_TO_C_STR_MAX_LENGTH     (4 * KILOBYTE)
const char *temp_String_To_C_Str(String s) {
    local_persist char buffers[TEMP_STRING_TO_C_STR_NUM_BUFFERS][TEMP_STRING_TO_C_STR_MAX_LENGTH];
    local_persist u32  current_buffer_index = 0;

    ASSERT(s.length + 1 < TEMP_STRING_TO_C_STR_MAX_LENGTH && "Your String is to big to fit into a temporary buffer");

    char *buf = buffers[current_buffer_index];
    current_buffer_index = (current_buffer_index + 1) % TEMP_STRING_TO_C_STR_NUM_BUFFERS;

    Mem_Copy(buf, s.data, s.length);
    buf[s.length] = 0;

    return buf;
}


String _String_Duplicate(String s, String_Duplicate_Opt opt) {
    String result = { .length = s.length, };
    u64 alloc_size = s.length + (opt.null_terminate ? 1 : 0);
    if (opt.allocator) {
        result.data = (char*) Arena_Alloc(opt.allocator, alloc_size);
    } else {
        result.data = (char*) BESTED_MALLOC(alloc_size);
    }
    ASSERT(result.data);
    Mem_Copy(result.data, s.data, result.length);
    // dont need this because the Arena_Alloc dose it for us,
    // but this assignment is kinda free. (the branch isn't though)
    if (opt.null_terminate) result.data[result.length] = 0;
    return result;
}

void String_To_Upper(String *s) {
    for (u64 i = 0; i < s->length; i++) s->data[i] = To_Upper(s->data[i]);
}

bool String_Eq(String s1, String s2) {
    if (s1.length != s2.length) return false;
    return Mem_Eq(s1.data, s2.data, s1.length);
}

bool String_Starts_With(String s, String prefix) {
    if (s.length < prefix.length) return false;
    return Mem_Eq(s.data, prefix.data, prefix.length);
}

bool String_Ends_With(String s, String postfix) {
    if (s.length < postfix.length) return false;
    return Mem_Eq(s.data + s.length - postfix.length, postfix.data, postfix.length);
}

bool String_Contains_Char(String s, char c) {
    for (u64 i = 0; i < s.length; i++) {
        if (s.data[i] == c) return true;
    }
    return false;
}

s64 String_Find_Index_Of_Char(String s, char c) {
    for (u64 i = 0; i < s.length; i++) {
        // this could break if we have a string that is 2^63 characters long.
        // oh the tragedy.
        if (s.data[i] == c) return i;
    }
    return -1;
}

s64 String_Find_Index_Of(String s, String needle) {
    if (needle.length == 0) return -1;

    if (needle.length == 1) return String_Find_Index_Of_Char(s, needle.data[0]);

    s64 absolute_index = 0;

    while (true) {
        s64 index = String_Find_Index_Of_Char(s, needle.data[0]);
        if (index == -1) return -1;

        absolute_index += index;

        // check if not enough room for needle
        if (s.length - index < needle.length) return -1;

        bool flag = true;
        for (u64 i = 1; i < needle.length; i++) {
            if (s.data[index+i] != needle.data[i]) {
                flag = false;
                break;
            }
        }

        if (flag) return absolute_index;

        absolute_index += 1;

        s.data   += index + 1;
        s.length -= index + 1;


    }

    return -1;
}

String String_get_single_line(String s, u64 index) {
    // clamp i if its to big. this handles a lot more errors than your thinking about.
    if (index > s.length) index = s.length;

    String result = String_Advanced(s, index);

    s64 index_of_end = String_Find_Index_Of_Char(result, '\n');

    // clamp the length to the far newline
    if (index_of_end != -1) { result.length = index_of_end; }

    // go back until newline before result.data,
    // and make sure it doesn't go out of bounds
    while (result.data != s.data && result.data[-1] != '\n') {
        String_Advance(&result, -1);
    }

    return result;
}

void   String_Advance (String *s, s64 count) {
    // don't do anything to null strings.
    if (s->data == NULL) return;
    // don't go over the length
    if ((s64)s->length < count) {
        String_Advance(s, s->length); // this'll get compiled out.
    } else {
        s->data   += count;
        s->length -= count;
    }
}
String String_Advanced(String s, s64 count) {
    String_Advance(&s, count);
    return s;
}

String String_Trim_Right(String s) {
    while (s.length > 0 && Is_Whitespace(s.data[s.length-1])) {
        s.length -= 1;
    }
    return s;
}

String String_Chop_While(String s, char_to_bool_func test_char_function) {
    u64 i;
    for (i = 0; i < s.length; i++) {
        if (!test_char_function(s.data[i])) break;
    }
    return String_Advanced(s, (s64)i);
}


Split_Once_Result Split_Once(String string, String separator) {
    s64 index = String_Find_Index_Of(string, separator);

    if (index == -1) {
        Split_Once_Result result = {
            .left  = string,
            .right = ZEROED,
            .ok    = false,
        };
        return result;
    } else {
        Split_Once_Result result = {
            .left  = { .data = string.data, .length = index },
            .right = {
                .data   = string.data   +  index + separator.length ,
                .length = string.length - (index + separator.length),
            },
            .ok    = true,
        };
        return result;
    }
}


u64 String_Split_By(String string, String separator, String_Array *result) {
    u64 split_count = 0;
    while (string.length > 0) {
        split_count += 1;
        Split_Once_Result split = Split_Once(string, separator);

        Array_Append(result, split.left);
        string = split.right;
    }
    return split_count;
}



String String_Path_to_Filename(String s) {
    while (true) {
        s64 index = String_Find_Index_Of_Char(s, '/');
        if (index == -1) break;
        String_Advance(&s, index+1);
    }
    return s;
}

String String_Remove_Extention(String s) {
    for (s64 i = s.length; i >= 0; i--) {
        if (s.data[i] == '.') {
            s.length = i;
            return s;
        }
    }
    return s;
}


String String_Get_Next_Line(String *parseing, u64 *line_num, String_Get_Next_Line_Flag flags) {
    bool remove_comments = Flag_Exists(flags, SGNL_Remove_Comments);
    bool trim            = Flag_Exists(flags, SGNL_Trim);
    bool skip_empty      = Flag_Exists(flags, SGNL_Skip_Empty);

    String next_line = *parseing;

    while (parseing->length > 0) {
        s64 line_end = String_Find_Index_Of_Char(*parseing, '\n');

        next_line = *parseing;

        if (line_end != -1) {
            next_line.length = (u64)line_end;
            String_Advance(parseing, line_end+1);
        } else {
            String_Advance(parseing, (s64)parseing->length);
        }

        if (line_num) *line_num += 1;

        if (remove_comments) {
            s64 comment_index = String_Find_Index_Of(next_line, S("//"));
            if (comment_index != -1) { next_line.length = (u64)comment_index; }
        }

        if (trim) next_line = String_Trim_Right(next_line);

        if ((!skip_empty) || (next_line.length > 0)) break;
    }

    return next_line;
}


#define TEMP_SPRINTF_NUM_BUFFERS    (1<<6)
static_assert(Is_Pow_2(TEMP_SPRINTF_NUM_BUFFERS), "for well defined wrapping behaviour");

#define TEMP_SPRINTF_BUFFER_SIZE    512 // pretty sure this number is related to page size...

const char *temp_sprintf(const char *format, ...) {
    local_persist char          buffers[TEMP_SPRINTF_NUM_BUFFERS][TEMP_SPRINTF_BUFFER_SIZE];
    local_persist Atomic(u32)   current_buffer_index = 0;

    u32 buffer_index = Atomic_Add(&current_buffer_index, 1);
    char *buf = buffers[buffer_index % Array_Len(buffers)];

    va_list args;
    va_start(args, format);
        vsnprintf(buf, sizeof(buffers[0]), format, args);
    va_end(args);

    return buf;
}

String temp_String_sprintf(const char *format, ...) {
    local_persist char          buffers[TEMP_SPRINTF_NUM_BUFFERS][TEMP_SPRINTF_BUFFER_SIZE];
    local_persist Atomic(u32)   current_buffer_index = 0;

    u32 buffer_index = Atomic_Add(&current_buffer_index, 1);
    char *buf = buffers[buffer_index % Array_Len(buffers)];

    va_list args;
    va_start(args, format);
        vsnprintf(buf, sizeof(buffers[0]), format, args);
    va_end(args);

    // could be faster because we know how long it is. but w/e
    return String_From_C_Str(buf);
}



// ===================================================
//                    String Builder
// ===================================================

internal Character_Buffer *String_Builder_Internal_Maybe_Expand_To_Fit(String_Builder *sb, u64 size) {
    // set the current segment to the first segment if its null (aka just after zero initialization)
    if (sb->current_segment == NULL) sb->current_segment = &sb->first_segment_holder;

    Character_Buffer *last_buffer = &sb->current_segment->buffers[sb->buffer_index % STRING_BUILDER_NUM_BUFFERS];
    while (true) {
        // if the segment hasn't been allocated
        if (last_buffer->capacity == 0) break;

        // if the segent has enough room to hold the new thing
        if (size + last_buffer->count <= last_buffer->capacity) break;

        // else move onto the next segment
        sb->buffer_index += 1;

        if (sb->buffer_index % STRING_BUILDER_NUM_BUFFERS == 0) {
            if (!sb->current_segment->next) {
                if (sb->allocator) {
                    sb->current_segment->next = (Segment*) Arena_Alloc_Struct(sb->allocator, Segment);
                } else {
                    sb->current_segment->next = (Segment*) BESTED_MALLOC(sizeof(Segment));
                    Mem_Zero(sb->current_segment->next, sizeof(Segment));
                }

                if (!sb->current_segment->next) {
                    STRING_BUILDER_PANIC("String Builder - maybe_expand_to_fit: failed to malloc a new segment holder");
                    return NULL;
                }
            }

            sb->current_segment = sb->current_segment->next;
        }

        last_buffer = &sb->current_segment->buffers[sb->buffer_index % STRING_BUILDER_NUM_BUFFERS];
    }


    if (last_buffer->capacity == 0) {
        u64 default_size = sb->base_new_allocation ? sb->base_new_allocation : STRING_BUILDER_BUFFER_DEFAULT_SIZE;
        u64 to_add_size = Max(size, default_size);

        if (sb->allocator) {
            last_buffer->data = (char*) Arena_Alloc(sb->allocator, to_add_size * sizeof(char), .clear_to_zero = true);
            // last_buffer->data = (char*) Arena_Alloc_Clear(sb->allocator, to_add_size * sizeof(char), true);
        } else {
            last_buffer->data = (char*) BESTED_MALLOC(to_add_size * sizeof(char));
            Mem_Zero(last_buffer->data, to_add_size * sizeof(char));
        }

        if (!last_buffer->data) {
            STRING_BUILDER_PANIC("maybe_expand_to_fit: failed to malloc a new segment with to_add_size");
            return NULL;
        }

        last_buffer->count = 0;
        last_buffer->capacity = to_add_size;
    }

    return last_buffer;
}

// this is not the most efficient thing ever.
// O(n^2) when looping over the entire array like this...
// but it will probably never come up as a performance problem.
internal inline Character_Buffer *String_Builder_Internal_Get_Character_Buffer_At_Index(String_Builder *sb, u64 buffer_index) {
    Segment *segment = &sb->first_segment_holder;
    while (buffer_index >= STRING_BUILDER_NUM_BUFFERS) {
        buffer_index -= STRING_BUILDER_NUM_BUFFERS;
        segment = segment->next;
    }
    return &segment->buffers[buffer_index];
}


u64 String_Builder_Count(String_Builder *sb) {
    u64 count = 0;
    for (u64 i = 0; i <= sb->buffer_index; i++) {
        Character_Buffer *buffer = String_Builder_Internal_Get_Character_Buffer_At_Index(sb, i);
        count += buffer->count;
    }
    return count;
}

u64 String_Builder_Total_Capacity(String_Builder *sb) {
    u64 capacity = 0;
    Segment *current_segment = &sb->first_segment_holder;
    while (current_segment) {
        for (u32 i = 0; i < STRING_BUILDER_NUM_BUFFERS; i++) {
            Character_Buffer *buffer = &current_segment->buffers[i];
            capacity += buffer->capacity;
        }
        current_segment = current_segment->next;
    }
    return capacity;
}

void String_Builder_Clear(String_Builder *sb) {
    for (u64 i = 0; i <= sb->buffer_index; i++) {
        Character_Buffer *buffer = String_Builder_Internal_Get_Character_Buffer_At_Index(sb, i);
        buffer->count = 0;
    }

    sb->buffer_index = 0;
    sb->current_segment  = &sb->first_segment_holder;
}

void String_Builder_Ptr_And_Size(String_Builder *sb, void *ptr, u64 size) {
    if (size == 0) return;

    Character_Buffer *buffer = String_Builder_Internal_Maybe_Expand_To_Fit(sb, size);

    if (!buffer) return; // String_Builder_Internal_Maybe_Expand_To_Fit() has already raised the panic.

    Mem_Copy(buffer->data + buffer->count, ptr, size);
    buffer->count += size;
}

void String_Builder_String(String_Builder *sb, String s) {
    String_Builder_Ptr_And_Size(sb, s.data, s.length);
}

u64 String_Builder_printf(String_Builder *sb, const char *format, ...) {
    va_list args;
    va_start(args, format);
        // TODO figure out how to do the thing we did in Arena.h for its sprintf
        s32 formatted_size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    // < 0 is an error from printf
    ASSERT(formatted_size >= 0);

    // early out.
    if (formatted_size == 0) return 0;

    // +1 because printf also puts a trailing '\0' byte.
    // we ignore that for the rest of the code.
    Character_Buffer *buffer = String_Builder_Internal_Maybe_Expand_To_Fit(sb, (u64)formatted_size+1);

    if (!buffer) return 0; // maybe_expand_to_fit has already raised the panic.

    va_start(args, format);
        vsprintf(buffer->data + buffer->count, format, args);
    va_end(args);

    buffer->count += (u64)formatted_size;
    return (u64)formatted_size;
}

String String_Builder_To_String(String_Builder *sb) {
    u64 count = String_Builder_Count(sb);

    String result = {
        .data   = NULL,
        .length = count,
    };

    if (sb->allocator) {
        result.data = (char*) Arena_Alloc(sb->allocator, count+1, .clear_to_zero = false);
        // result.data = (char*) Arena_Alloc_Clear(sb->allocator, count+1, false);
    } else {
        result.data = (char*) BESTED_MALLOC(count+1);
    }

    if (!result.data) {
        STRING_BUILDER_PANIC("Arena_Alloc failed when trying to allocate enough memory to hold to result string from String_Builder_To_String()");
        result.length = 0;
        return result;
    }

    u64 index = 0;
    for (u64 i = 0; i <= sb->buffer_index; i++) {
        Character_Buffer *buffer = String_Builder_Internal_Get_Character_Buffer_At_Index(sb, i);
        Mem_Copy(result.data + index, buffer->data, buffer->count);
        index += buffer->count;
    }

    result.data[result.length] = 0;
    return result;
}

void String_Builder_To_File(String_Builder *sb, FILE *file) {
    for (u64 i = 0; i <= sb->buffer_index; i++) {
        Character_Buffer *buffer = String_Builder_Internal_Get_Character_Buffer_At_Index(sb, i);
        fwrite(buffer->data, sizeof(char), buffer->count, file);
    }
}

void String_Builder_Free(String_Builder *sb) {
    if (sb->allocator) {
            fprintf(stderr, "=======================================================================================\n");
            fprintf(stderr, "Are you serious?\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "Did you just attempt to free a string buffer that was allready given an allocator?\n");
            fprintf(stderr, "Not just any allocator either, the only thing string builders accept are arena allocators.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "You know I do this for you right?\n");
            fprintf(stderr, "I give you all these tools and this is what you do with it?\n");
            fprintf(stderr, "Make a mistake that could have easily been ignored, look into this function you just called.\n");
            fprintf(stderr, "I can check if you have an allocator and just ignore it, but I wont.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "I'm not gonna even ASSERT(false).\n");
            fprintf(stderr, "I'm gonna let the segmentation falt, or the subtle memory bug do the talking for me.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "I hope you have a terrible day.\n");
            fprintf(stderr, "=======================================================================================\n");
    }

    Segment *segment = &sb->first_segment_holder;
    bool first_segment = true;

    while (segment) {
        for (u32 i = 0; i < STRING_BUILDER_NUM_BUFFERS; i++) {
            Character_Buffer *buffer = &segment->buffers[i];
            if (buffer->data) BESTED_FREE(buffer->data);

            // only matters if its the first segment.
            buffer->count    = 0;
            buffer->capacity = 0;
            buffer->data     = NULL;
        }

        Segment *next_segment = segment->next;

        if (!first_segment) BESTED_FREE(segment);
        first_segment = false;

        segment = next_segment;
    }

    sb->first_segment_holder.next = NULL;
    sb->current_segment = &sb->first_segment_holder;
}



// ===================================================
//             Some Common Functions
// ===================================================

String Read_Entire_File(String filename, Arena *arena) {
    String result = ZEROED;

    // pretty sure PATHMAX is less than TEMP_STRING_TO_C_STR_MAX_LENGTH
    if (filename.length >= TEMP_STRING_TO_C_STR_MAX_LENGTH) {
        fprintf(stderr, "filename length is bigger than temp_String_To_C_Str() storage, was %zu, witch is probably bigger than PATH_MAX on a lot of OS's", filename.length);
        return result;
    }

    FILE *file = fopen(temp_String_To_C_Str(filename), "rb");

    if (file) {
        fseek(file, 0, SEEK_END);
        s64 length = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (length >= 0) {
            result.length = (u64)length;
            if (arena) {
                result.data = (char*) Arena_Alloc(arena, result.length+1, .clear_to_zero = false);
            } else {
                result.data = BESTED_MALLOC(result.length+1);
            }
            // result.data = (char*) Arena_Alloc_Clear(arena, result.length+1, false);
            if (result.data) {
                u64 read_bytes = fread(result.data, 1, result.length, file);
                ASSERT(read_bytes == result.length);
                result.data[result.length] = 0; // zero terminate for the love of the game.
            }
        }

        fclose(file);
    }

    return result;
}



#ifdef _WIN32
    #warning "We dont support windows currently, or at least not nanoseconds_since_unspecified_epoch(), returns a dummy value."
#else
    #include <unistd.h>
    #include <time.h>
#endif


u64 nanoseconds_since_unspecified_epoch(void) {
#ifdef __unix__
    struct timespec ts;

    #ifndef CLOCK_MONOTONIC
        #define VSCODE_IS_DUMB
        #define CLOCK_MONOTONIC 69420
    #endif
    // dont worry, this will never trigger.
    ASSERT(CLOCK_MONOTONIC != 69420);

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return NANOSECONDS_PER_SECOND * ts.tv_sec + ts.tv_nsec;
#else
    #warning "We dont support windows currently, or at least not nanoseconds_since_unspecified_epoch(), returns a dummy value."
    return 538008;
#endif
}


// https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
bool check_if_file_exists(const char *filepath) {
    return access(filepath, F_OK) == 0;
}



// ===================================================
//                 Debug Helpers
// ===================================================

const char *print_s64   (void *_x) { s64 x    = *(s64*)   _x; return temp_sprintf("%ld", x); }
const char *print_u64   (void *_x) { u64 x    = *(u64*)   _x; return temp_sprintf("%ld", x); }

const char *print_s32   (void *_x) { s32 x    = *(s32*)   _x; return temp_sprintf("%d",  x); }
const char *print_u32   (void *_x) { u32 x    = *(u32*)   _x; return temp_sprintf("%d",  x); }

const char *print_s16   (void *_x) { s16 x    = *(s16*)   _x; return temp_sprintf("%d",  x); }
const char *print_u16   (void *_x) { u16 x    = *(u16*)   _x; return temp_sprintf("%d",  x); }

const char *print_s8    (void *_x) { s8  x    = *(s8 *)   _x; return temp_sprintf("%d",  x); }
const char *print_u8    (void *_x) { u8  x    = *(u8 *)   _x; return temp_sprintf("%d",  x); }

const char *print_f32   (void *_x) { f32 x    = *(f32*)   _x; return temp_sprintf("%f",  x); }
const char *print_f64   (void *_x) { f64 x    = *(f64*)   _x; return temp_sprintf("%f",  x); }


const char *print_bool  (void *_x) { bool x   = *(bool*)  _x; return x ? "true" : "false"; }

const char *print_string(void *_x) { String x = *(String*)_x; return temp_sprintf("\""S_Fmt"\"", S_Arg(x)); }





#endif // BESTED_IMPLEMENTATION


#ifdef __cplusplus
    }
#endif


