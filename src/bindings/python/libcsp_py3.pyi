
import ctypes as _ctypes
from typing import Any as _Any


# Source: https://stackoverflow.com/questions/60310292/how-to-check-if-an-object-in-python-is-a-pycapsule
def _get_capsule_type() -> _Any:
    class PyTypeObject(_ctypes.Structure):
        pass  # don't need to define the full structure
    capsuletype = PyTypeObject.in_dll(_ctypes.pythonapi, "PyCapsule_Type")
    capsuletypepointer = _ctypes.pointer(capsuletype)
    return _ctypes.py_object.from_address(_ctypes.addressof(capsuletypepointer)).value

# Type-hinting PyCapsule* is not pretty,
#   still it's the closest we can get to type-hinting a CSP packet or connection.
_PyCapsule = _get_capsule_type()


# Custom Exceptions
class Error(Exception):
    ...


# RESERVED PORTS
CSP_CMP: int
CSP_PING: int
CSP_PS: int
CSP_MEMFREE: int
CSP_REBOOT: int
CSP_BUF_FREE: int
CSP_UPTIME: int
CSP_ANY: int

# PRIORITIES
CSP_PRIO_CRITICAL: int
CSP_PRIO_HIGH: int
CSP_PRIO_NORM: int
CSP_PRIO_LOW: int

# FLAGS
CSP_FFRAG: int
CSP_FHMAC: int
CSP_FRDP: int
CSP_FCRC32: int

# SOCKET OPTIONS
CSP_SO_NONE: int
CSP_SO_RDPREQ: int
CSP_SO_RDPPROHIB: int
CSP_SO_HMACREQ: int
CSP_SO_HMACPROHIB: int
CSP_SO_CRC32REQ: int
CSP_SO_CRC32PROHIB: int
CSP_SO_CONN_LESS: int

# CONNECT OPTIONS
CSP_O_NONE: int
CSP_O_RDP: int
CSP_O_NORDP: int
CSP_O_HMAC: int
CSP_O_NOHMAC: int
CSP_O_CRC32: int
CSP_O_NOCRC32: int

# csp/csp_error.h
CSP_ERR_NONE: int
CSP_ERR_NOMEM: int
CSP_ERR_INVAL: int
CSP_ERR_TIMEDOUT: int
CSP_ERR_USED: int
CSP_ERR_NOTSUP: int
CSP_ERR_BUSY: int
CSP_ERR_ALREADY: int
CSP_ERR_RESET: int
CSP_ERR_NOBUFS: int
CSP_ERR_TX: int
CSP_ERR_DRIVER: int
CSP_ERR_AGAIN: int
CSP_ERR_HMAC: int
CSP_ERR_CRC32: int
CSP_ERR_SFP: int

# Misc
CSP_NO_VIA_ADDRESS: int
CSP_MAX_TIMEOUT: int


# csp/csp.h
def service_handler(conn: _PyCapsule, packet: _PyCapsule) -> None:
    ...
def init(hostname: str, model: str, revision: str, version: int = ..., conn_dfl_so: int = ..., dedup: int = ...) -> None:
    ...
def get_hostname() -> str:
    ...
def get_model() -> str:
    ...
def get_revision() -> str:
    ...
def socket(opts: int = CSP_SO_NONE) -> _PyCapsule:
    ...
def accept(socket: _PyCapsule, timeout: int = CSP_MAX_TIMEOUT) -> None | _PyCapsule:
    ...
def read(conn: _PyCapsule, timeout: int = 500) -> None | _PyCapsule:
    ...
def send(conn: _PyCapsule, packet: _PyCapsule, timeout: int = 1000) -> None:
    ...
def transaction(prio: int, dest: int, port: int, timeout: int, inbuf: bytes, outbuf: bytes, allow_any_incoming_length: int = 0) -> int:
    ...
def sendto_reply(request_packet: _PyCapsule, reply_packet: _PyCapsule, opts: int = CSP_O_NONE) -> None:
    ...
def sendto(prio: int, dest: int, dport: int, src_port: int, opts: int, packet: _PyCapsule) -> None:
    ...
def recvfrom(socket: _PyCapsule, timeout: int = 500) -> _PyCapsule:
    ...
def connect(prio: int, dest: int, dport: int, timeout: int, opts: int) -> _PyCapsule:
    ...
def close(conn: _PyCapsule) -> int:
    ...
def conn_dport(conn: _PyCapsule) -> int:
    ...
def conn_sport(conn: _PyCapsule) -> int:
    ...
def conn_dst(conn: _PyCapsule) -> int:
    ...
def conn_src(conn: _PyCapsule) -> int:
    ...
def listen(socket: _PyCapsule, conn_queue_length: int = 10) -> None:
    ...
def bind(socket: _PyCapsule, port: int) -> None:
    ...
def route_start_task()  -> None:
    ...
def ping(node: int, timeout: int = 1000, size: int = 10, conn_options: int = CSP_O_NONE) -> int:
    ...
def reboot(node: int) -> None:
    ...
def shutdown(node: int) -> None:
    ...
def rdp_set_opt(window_size: int, conn_timeout_ms: int, packet_timeout_ms: int, delayed_acks: int, ack_timeout: int, ack_delay_count: int) -> None:
    ...
def rdp_get_opt() -> tuple[int, int, int, int, int, int]:
    ...


if True:  # CSP_USE_RTABLE:
    def rtable_set(node: int, mask: int, interface_name: str, via: int = CSP_NO_VIA_ADDRESS) -> None:
        ...
    def rtable_clear() -> None:
        ...
    def rtable_check(buffer: str) -> int:
        ...
    def rtable_load(buffer: str) -> int:
        ...
    def print_routes() -> None:
        ...


# csp/csp_buffer.h
def buffer_free(packet: _PyCapsule) -> None:
    ...
def buffer_get() -> _PyCapsule:
    ...
def buffer_remaining() -> int:
    ...


# csp/csp_cmp.h
def cmp_ident(node: int, timeout: int = 1000) -> tuple[int, str, str, str, str, str]:
    ...
def cmp_route_set(node: int, timeout: int, addr: int, via: int, ifstr: str) -> None:
    ...
def cmp_peek(node: int, timeout: int, addr: int, len: int, outbuf: bytes) -> None:
    ...
def cmp_poke(node: int, timeout: int, len: int, addr: int, inbuf: bytes) -> None:
    ...
def cmp_clock_set(node: int, sec: int, nsec: int, timeout: int = 1000) -> None:
    ...
def cmp_clock_get(node: int, timeout: int = 1000) -> tuple[int, int]:
    ...


if True: # CSP_HAVE_LIBZMQ:
    def zmqhub_init(addr: int, host: str) -> None:
        ...


def kiss_init(device: str, baudrate: int = 500000, mtu: int = 512, addr: int = 0, if_name: str = "KISS") -> None:
    ...


# csp/drivers/can_socketcan.h
def can_socketcan_init(ifc: str, bitrate: int = 1000000, promisc: int = 0, addr: int = 0) -> None:
    ...


# Helpers
def packet_get_length(packet: _PyCapsule) -> int:
    ...
def packet_get_data(packet: _PyCapsule) -> bytes:
    ...
def packet_set_data(packet: _PyCapsule, data: bytes) -> None:
    ...
def print_connections() -> None:
    ...
def print_interfaces() -> None:
    ...
