#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// An open descriptor for a runtime file handle.
typedef int32_t descriptor;

/// Options for creating the file.
struct create_options {
    /// Object ID to bind the name to, optional. 0 if not present.
    objid id;
    /// The kind of open/create operation. See CREATE_KIND_*.
    uint8_t kind;
};

const size_t NAME_DATA_MAX = 2048;

/// Information for opening a file.
struct open_info {
  /// Creation options
  struct create_options create;
  /// Operation flags
  uint32_t flags;
  /// Length of file name in bytes.
  size_t len;
  uint8_t name[NAME_DATA_MAX];
};

/// Open the file only if it already exists.
const uint8_t CREATE_KIND_EXISTING = 0;
/// Open only if it doesn't exist, and create it.
const uint8_t CREATE_KIND_NEW = 1;
/// Open if it already exists, or create it if it doesn't.
const uint8_t CREATE_KIND_EITHER = 2;

/// Open the file with read access.
const uint32_t OPEN_FLAG_READ = 1;
/// Open the file with write access.
const uint32_t OPEN_FLAG_WRITE = 2;
/// Truncate the file on open. Requires write access.
const uint32_t OPEN_FLAG_TRUNCATE = 4;
/// Always use the end of the file as the position.
const uint32_t OPEN_FLAG_TAIL = 8;
/// If the file is a symlink, open the link instead of the target.
const uint32_t OPEN_FLAG_SYMLINK = 0x10;

/// Result of open call.
struct open_result {
  /// If error is Success, this contains a valid descriptor.
  descriptor fd;
  /// Error code, or success.
  twz_error err;
};

enum open_kind {
  OpenKind_KernelConsole,
  OpenKind_Object,
  OpenKind_Path,
  OpenKind_Pipe,
  OpenKind_SocketConnect,
  OpenKind_SocketBind,
  OpenKind_SocketAccept,
  OpenKind_PtyServer,
  OpenKind_PtyClient,
  OpenKind_Compartment,
};

enum addr_kind {
  AddrKind_Ipv4,
  AddrKind_Ipv6,
};

enum prot_kind {
    ProtKind_Stream,
    ProtKind_Datagram,
};

union socket_address_addrs {
  uint8_t v4[4];
  uint8_t v6[16];
};

struct socket_address {
  enum addr_kind kind;
  union socket_address_addrs addr_octets;
  uint16_t port;
  uint32_t scope_id;
  uint32_t flowinfo;
};

struct socket_bind_info {
    struct socket_address addr;
    enum prot_kind prot;
};

struct object_bind_info {
    objid id;
};

/// Open a non-named file. The value pointed to by bind_info is dependent on the kind specified in the first
/// argument. For pipe, bind_info is ignored. For Socket* kinds, bind_info points to a socket_address.
extern struct open_result twz_rt_fd_open(enum open_kind kind, uint32_t flags, void *bind_info, size_t bind_info_len);

/// Reopen a file descriptor with a new anon binding. The anon_kind remains unchanged. The value pointed to by bind_info is dependent on the kind specified in the first
/// argument. For pipe, bind_info is ignored. For Socket* kinds, bind_info points to a socket_address.
extern twz_error twz_rt_fd_reopen(descriptor fd, enum open_kind kind, uint32_t flags, void *bind_info, size_t bind_info_len);

/// Close a file descriptor. If the file descriptor is invalid
/// or already closed, this function does nothing.
extern void twz_rt_fd_close(descriptor fd);

/// Flags a descriptor can have.
typedef uint32_t fd_flags;

/// This file descriptor is a terminal.
const fd_flags FD_IS_TERMINAL = 1;

/// Kinds of underlying fd objects
enum fd_kind {
  /// Regular file
  FdKind_Regular,
  /// Directory
  FdKind_Directory,
  /// Symbolic link
  FdKind_SymLink,
  FdKind_Socket,
  FdKind_Pipe,
  FdKind_Pty,
  FdKind_Compartment,
};

/// Information about a file descriptor.
struct fd_info {
  /// Underlying root objid.
  objid id;
  /// Length of underlying object, or 0 if undefined.
  uint64_t len;
  /// Flags for the descriptor.
  fd_flags flags;
  /// Underlying fd kind
  enum fd_kind kind;
  struct duration created;
  struct duration accessed;
  struct duration modified;
  uint32_t unix_mode;
};
/// Get information about a descriptor. If this returns true, the fd was valid
/// and the data pointed to by info is filled with fd_info data.
extern bool twz_rt_fd_get_info(descriptor fd, struct fd_info *info);

/// Commands for descriptors.
typedef uint32_t fd_cmd;

/// Duplicate this descriptor. The arg argument is ignored. The ret argument points to a descriptor.
const fd_cmd FD_CMD_DUP = 0;
/// Sync the underlying storage of the file descriptor.
const fd_cmd FD_CMD_SYNC = 1;
/// Truncate the underlying storage of the file descriptor. The arg argument points to a u64 length.
const fd_cmd FD_CMD_TRUNCATE = 2;
/// Close either the read or write end of a file descriptor. The arg points to a u32, the first bit of which indicates read-side, the second indicates write.
const fd_cmd FD_CMD_SHUTDOWN = 3;

/// Perform a command on the descriptor. The arguments arg and ret are interpreted according to
/// the command specified.
extern twz_error twz_rt_fd_cmd(descriptor fd, fd_cmd cmd, void *arg, void *ret);

const size_t BIND_DATA_MAX = 4096;

struct binding_info {
    enum open_kind kind;
    descriptor fd;
    fd_flags flags;
    uint32_t bind_len;
    uint8_t bind_data[BIND_DATA_MAX];
} __attribute__ ((aligned (16)));

extern size_t twz_rt_fd_read_binds(struct binding_info *binds, size_t nr_binds);

#define NAME_ENTRY_LEN 256
struct name_entry {
  struct fd_info info;
  uint32_t name_len;
  uint32_t linkname_len;
  uint8_t name[NAME_ENTRY_LEN];
};

/// Enumerate sub-names in an fd (e.g. directory entries). The buf and len arguments form a &mut [name_entry] slice, and the off argument specifies how many names to skip for this read. The return value is the number of entries read, or
/// 0 if at end of list.
extern struct io_result twz_rt_fd_enumerate_names(descriptor fd, struct name_entry *buf, size_t len, size_t off);

/// Remove a name in the namespace.
extern twz_error twz_rt_fd_remove(const char *name, size_t name_len);

/// Create a new namespace.
extern twz_error twz_rt_fd_mkns(const char *name, size_t name_len);

/// Create a new symlink.
extern twz_error twz_rt_fd_symlink(const char *name, size_t name_len, const char *target, size_t target_len);

/// Read symlink.
extern twz_error twz_rt_fd_readlink(const char *name, size_t name_len, char *buf, size_t buf_len, uint64_t *out_buf_len);

enum name_root {
    NameRoot_Root,
    NameRoot_Home,
    NameRoot_Current,
    NameRoot_Temp,
    NameRoot_Exe,
};

extern twz_error twz_rt_set_nameroot(enum name_root root, const char *path, size_t path_len);

extern struct io_result twz_rt_get_nameroot(enum name_root root, char *path, size_t path_len);

enum name_resolver {
    NameResolver_Default,
    NameResolver_Socket,
};

extern struct objid_result twz_rt_resolve_name(enum name_resolver resolver, const char *name, size_t name_len);

extern twz_error twz_rt_canon_name(enum name_resolver resolver, const char *name, size_t name_len, char *out, size_t *out_len);

#ifdef __cplusplus
}
#endif
