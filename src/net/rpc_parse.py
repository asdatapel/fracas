import os
import sys

if len(sys.argv) != 3:
    print('Wrong number of args')
    exit()

in_file = open(sys.argv[1], 'r')

objects = in_file.read().split('\n\n')


builtins = {'int': 'int32_t', 'uint': 'uint32_t', 'string': "AllocatedString<64>", 'list': "std::vector"}
defaults = {'int': '0', 'uint': '0', 'bool': 'false', 'string': '{}', 'list': '{}'}
messages = {}
rpcs = {}


def to_type(ts):
    if ts[0] == "list":
        return f"{builtins[ts[0]]}<{to_type(ts[1:])}>"
    return builtins[ts[0]] if ts[0] in builtins else ts[0]

def get_default(type):
    if type in defaults:
        return defaults[type]
    return '{}'


def parse_message(str):
    lines = str.splitlines()
    name = lines[0].split(' ')[1]

    members = []
    for l in lines[1:]:
        parts = l.split(' ')
        members.append((parts[0], to_type(parts[1:])))
    messages[name] = members


def parse_rpc(str):
    lines = str.splitlines()
    name = lines[0].split(' ')[1]
    req = lines[1]
    resp = lines[2]

    rpcs[name] = (req, resp)

for obj in objects:
    if obj.startswith('message'):
        parse_message(obj)
    if obj.startswith('rpc'):
        parse_rpc(obj)

messages_text = ""
header = "#include \"net_windows.hpp\"\n"
header += "#include <vector>\n"
messages_text += header + '\n'

for name in messages:
    struct_def = f"struct {name} {{\n"
    for var in messages[name]:
        struct_def += f"\t{var[1]} {var[0]} = {get_default(var[1])}; \n"
    struct_def += f"}};\n"

    append_func = f"void append(MessageBuilder *msg, {name} &in) {{\n"
    for var in messages[name]:
        append_func += f"\tappend(msg, in.{var[0]});\n"
    append_func += f"}};\n"

    read_func = f"uint32_t read(MessageReader *msg, {name} *out) {{\n"
    read_func += f"\tuint32_t len = 0;\n\n"
    for var in messages[name]:
        read_func += f"\tlen += read(msg, &out->{var[0]});\n"
    read_func += f"\n\treturn len;\n"
    read_func += f"}};\n"

    list_append_func = f"void append(MessageBuilder *msg, std::vector<{name}> &in) {{\n"
    list_append_func += "\tappend(msg, (uint16_t)in.size());\n"
    list_append_func += f"\tfor (auto &it : in){{append(msg, it);}}\n"
    list_append_func += "};\n"

    list_read_func = f"uint32_t read(MessageReader *msg, std::vector<{name}> *out) {{\n"
    list_read_func += "\tuint32_t len = 0;\n\n"
    list_read_func += "\tuint16_t list_len;\n"
    list_read_func += "\tlen += read(msg, &list_len);\n"
    list_read_func += "\tfor (int i = 0; i < list_len; i++) {\n"
    list_read_func += f"\t\t{name} elem; len += read(msg, &elem); out->push_back(elem);\n"
    list_read_func += "\t}\n\n"
    list_read_func += "\treturn len;\n"
    list_read_func += "};\n"

    messages_text += struct_def + append_func + read_func + list_append_func + list_read_func + '\n'

enum_def = "enum struct Rpc : char {\n"
for i, rpc in enumerate(rpcs):
    rpc = rpc + " = 1" if i == 0 else rpc
    enum_def += f"\t{rpc},\n"
enum_def += "};\n"
messages_text += enum_def + "\n"

server_text = "#pragma once\n#include \"generated_messages.hpp\"\n\n"

server_def = "struct ServerData;\n"
server_def += "struct RpcServer {\n"
server_def += "\tServerData *server_data;\n\n"
server_def += "\tvoid handle_rpc(ClientId, Peer*, char*, int);\n"
for rpc in rpcs:
    server_def += f"\tvoid {rpc}(ClientId client_id, {rpcs[rpc][0]}*, {rpcs[rpc][1]}*);\n"
server_def += "};"
server_text += server_def + "\n"

handle_rpc_fun = "void RpcServer::handle_rpc(ClientId client_id, Peer *peer, char *data, int msg_len) {\n"
handle_rpc_fun += "\tRpc rpc_type;\n"
handle_rpc_fun += "\tdata = read_byte(data, (char *)&rpc_type);\n"
handle_rpc_fun += "\tMessageReader in(data, msg_len - 1);\n\n"
handle_rpc_fun += "\tMessageBuilder out;\n"
handle_rpc_fun += "\tswitch(rpc_type) {\n"
for rpc in rpcs:
    handle_rpc_fun += f"\t\tcase Rpc::{rpc}: {{\n"
    handle_rpc_fun += f"\t\t\t{rpcs[rpc][0]} req;\n"
    handle_rpc_fun += f"\t\t\t{rpcs[rpc][1]} resp;\n"
    handle_rpc_fun += "\t\t\tread(&in, &req);\n"
    handle_rpc_fun += f"\t\t\t{rpc}(client_id, &req, &resp);\n"
    handle_rpc_fun += "\t\t\tappend(&out, resp);\n"
    handle_rpc_fun += "\t\t\tout.send(peer);\n"
    handle_rpc_fun += "\t\t} break;\n"
handle_rpc_fun += "\t}\n"
handle_rpc_fun += "}\n"
server_text += handle_rpc_fun + "\n"

client_text = "#pragma once\n#include \"generated_messages.hpp\"\n\n"

client_def = "struct RpcClient {\n"
client_def += "\tPeer peer;\n"
client_def += "\tRpcClient(const char *address, uint16_t port) {\n"
client_def += "\t\tpeer.open(address, port, true);\n"
client_def += "\t}\n\n"
for rpc in rpcs:
    client_def += f"\tvoid {rpc}({rpcs[rpc][0]} &, {rpcs[rpc][1]}*);\n"
    client_def += f"\t{rpcs[rpc][1]} {rpc}({rpcs[rpc][0]});\n\n"
client_def += "};\n"
client_text += client_def + "\n"

client_func_defs = ""
for rpc in rpcs:
    client_func_defs += f"void RpcClient::{rpc}({rpcs[rpc][0]} &req, {rpcs[rpc][1]} *resp) {{\n"
    client_func_defs += f"\tMessageBuilder out; append(&out, (char) Rpc::{rpc}); append(&out, req); out.send(&peer);\n\n"
    client_func_defs += "\tint msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}\n"
    client_func_defs += "\tMessageReader in(msg, msg_len); read(&in, resp);\n"
    client_func_defs += "}\n"
    client_func_defs += f"{rpcs[rpc][1]} RpcClient::{rpc}({rpcs[rpc][0]} req) {{\n"
    client_func_defs += f"\t{rpcs[rpc][1]} resp;\n"
    client_func_defs += f"\t{rpc}(req, &resp);\n"
    client_func_defs += "\treturn resp;\n"
    client_func_defs += "}\n\n"
client_text += client_func_defs + "\n"

messages_file = open(sys.argv[2] + '/generated_messages.hpp', 'w+')
messages_file.write(messages_text)
messages_file.close()

server_file = open(sys.argv[2] + '/generated_rpc_server.hpp', 'w+')
server_file.write(server_text)
server_file.close()

client_file = open(sys.argv[2] + '/generated_rpc_client.hpp', 'w+')
client_file.write(client_text)
client_file.close()