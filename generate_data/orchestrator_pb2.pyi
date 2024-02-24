from google.protobuf import empty_pb2 as _empty_pb2
from google.protobuf.internal import containers as _containers
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class ApiVertex(_message.Message):
    __slots__ = ["key", "value"]
    KEY_FIELD_NUMBER: _ClassVar[int]
    VALUE_FIELD_NUMBER: _ClassVar[int]
    key: str
    value: str
    def __init__(self, key: _Optional[str] = ..., value: _Optional[str] = ...) -> None: ...

class ApiEdge(_message.Message):
    __slots__ = ["to", "label", "key"]
    FROM_FIELD_NUMBER: _ClassVar[int]
    TO_FIELD_NUMBER: _ClassVar[int]
    LABEL_FIELD_NUMBER: _ClassVar[int]
    KEY_FIELD_NUMBER: _ClassVar[int]
    to: str
    label: str
    key: str
    def __init__(self, to: _Optional[str] = ..., label: _Optional[str] = ..., key: _Optional[str] = ..., **kwargs) -> None: ...

class ApiGraphSummary(_message.Message):
    __slots__ = ["vertex_count", "edge_count"]
    VERTEX_COUNT_FIELD_NUMBER: _ClassVar[int]
    EDGE_COUNT_FIELD_NUMBER: _ClassVar[int]
    vertex_count: int
    edge_count: int
    def __init__(self, vertex_count: _Optional[int] = ..., edge_count: _Optional[int] = ...) -> None: ...

class ApiSearchArgs(_message.Message):
    __slots__ = ["query_key", "level"]
    QUERY_KEY_FIELD_NUMBER: _ClassVar[int]
    LEVEL_FIELD_NUMBER: _ClassVar[int]
    query_key: str
    level: int
    def __init__(self, query_key: _Optional[str] = ..., level: _Optional[int] = ...) -> None: ...

class ApiSearchResults(_message.Message):
    __slots__ = ["vertices", "edges"]
    VERTICES_FIELD_NUMBER: _ClassVar[int]
    EDGES_FIELD_NUMBER: _ClassVar[int]
    vertices: _containers.RepeatedCompositeFieldContainer[ApiVertex]
    edges: _containers.RepeatedCompositeFieldContainer[ApiEdge]
    def __init__(self, vertices: _Optional[_Iterable[_Union[ApiVertex, _Mapping]]] = ..., edges: _Optional[_Iterable[_Union[ApiEdge, _Mapping]]] = ...) -> None: ...
