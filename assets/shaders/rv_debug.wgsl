const _msg_ty_u32: u32 = 0;
const _msg_ty_i32: u32 = 1;
struct Message {
    value: vec4<u32>,
    ty: u32,
}
fn _dbg_v(_type: u32, _v: vec4<u32>) {
    if (messages_info.count >= 128) {
        return;
    }

    let index = messages_info.count;
    messages_info.count = index + 1;

    var m: Message;
    m.ty = _type;
    m.value = _v;

    messages[index] = m;
}
fn dbg_u32(_v: u32) {
    _dbg_v(_msg_ty_u32, vec4(_v, 0, 0, 0));
}
fn dbg_i32(_v: i32) {
    _dbg_v(_msg_ty_i32, vec4(bitcast<u32>(_v), 0, 0, 0));
}

struct MessageInfo {
    count: u32,
}

@group(2) @binding(0) var<storage, read_write> messages: array<Message>;
@group(2) @binding(1) var<storage, read_write> messages_info: MessageInfo;
