import os
import sys
from string import Template

message_file_template = Template("""
#pragma once
#include "net_windows.hpp"
#include <vector>

enum struct Message : char
{
$message_enum_values
};

$messages

enum struct Rpc : char
{
$rpc_enum_values
};
""")
message_template = Template("""
struct $name 
{
$members
};
void append(MessageBuilder *msg, $name &in)
{
$appends
}
uint32_t read(MessageReader *msg, $name *out)
{
    uint32_t len = 0;

$reads
    return len;
}
void append(MessageBuilder *msg, std::vector<$name> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<$name> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		$name elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};
""")
member_template = Template("""    $type $member_name = $default;""")
append_template = Template("""    append(msg, in.$member);""")
read_template = Template("""    len += read(msg, &out->$member);""")

client_file_template = Template("""
#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcClient : public BaseRpcClient
{
    using BaseRpcClient::BaseRpcClient;
    bool handle_rpc(char*, int);

$rpc_handler_decls

$callable_rpc_decls
};

bool RpcClient::handle_rpc(char *data, int msg_len)
{
    Rpc rpc_type;
    data = read_byte(data, (char *)&rpc_type);
    MessageReader in(data, msg_len - 1);
    switch(rpc_type) {
$handle_rpc_cases
        default:
        return false;
    }

    return true;
}

$callable_rpc_defs
""")
client_rpc_handler_decl_template = Template("""    void Handle$name($req*);""")
client_rpc_case_template = Template(
    """    case Rpc::Oneway$name:
        {
            $req req;
            read(&in, &req);
            Handle$name(&req);
        }
        break;""")
client_callable_rpc_template = Template("""
    void $name($req &, $resp*);
    $resp $name($req);
""")
client_callable_rpc_def_template = Template("""
void RpcClient::$name($req &req, $resp *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::$name);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::$name){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
$resp RpcClient::$name($req req) 
{
    $resp resp;
    $name(req, &resp);
    return resp;
}
""")

##################
server_file_template = Template("""
#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcServer : public BaseRpcServer
{
    using BaseRpcServer::BaseRpcServer;
    void handle_rpc(ClientId, Peer*, char*, int);

$rpc_handler_decls

$callable_rpc_decls
};

void RpcServer::handle_rpc(ClientId client_id, Peer *peer, char *data, int msg_len)
{
    Rpc rpc_type;
    data = read_byte(data, (char *)&rpc_type);
    MessageReader in(data, msg_len - 1);
    MessageBuilder out;
    switch(rpc_type) {
$handle_rpc_cases
    }
}

$callable_rpc_defs
""")
server_rpc_handler_decl_template = Template(
    """    void Handle$name(ClientId client_id, $req*, $resp*);""")
server_rpc_case_template = Template(
    """    case Rpc::$name:
        {
            $req req;
            $resp resp;
            read(&in, &req);
            Handle$name(client_id, &req, &resp);
            append(&out, (char)Rpc::$name);
            append(&out, resp);
            out.send(peer);
        }
        break;""")
server_callable_rpc_template = Template("""
    void $name(Peer *, $req);
""")
server_callable_rpc_def_template = Template("""
void RpcServer::$name(Peer *peer, $req req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::Oneway$name);
    append(&out, req); out.send(peer);
}
""")


builtins = {'int': 'int32_t', 'uint': 'uint32_t',
            'string': "AllocatedString<64>", 'list': "std::vector"}
defaults = {'int': '0', 'uint': '0',
            'bool': 'false', 'string': '{}', 'list': '{}'}


def to_type(ts):
    if ts[0] == "list":
        return f"{builtins[ts[0]]}<{to_type(ts[1:])}>"
    return builtins[ts[0]] if ts[0] in builtins else ts[0]


def get_default(type):
    if type in defaults:
        return defaults[type]
    return '{}'


class Message:
    name = ""
    members = []

    def __init__(self, name, members):
        self.name = name
        self.members = members

    def parse(str):
        lines = str.splitlines()
        name = lines[0].split(' ')[1]

        members = []
        for l in lines[1:]:
            parts = l.split(' ')
            members.append((parts[0], to_type(parts[1:])))

        return Message(name, members)


class Rpc:
    name = ""
    req_type = ""
    resp_type = ""

    def __init__(self, name, req_type, resp_type):
        self.name = name
        self.req_type = req_type
        self.resp_type = resp_type

    def parse(str):
        lines = str.splitlines()
        name = lines[0].split(' ')[1]
        req = lines[1]
        resp = lines[2]

        return Rpc(name, req, resp)


class Oneway:
    name = ""
    req_type = ""

    def __init__(self, name, req_type):
        self.name = name
        self.req_type = req_type

    def parse(str):
        lines = str.splitlines()
        name = lines[0].split(' ')[1]
        req = lines[1]

        return Oneway(name, req)


if len(sys.argv) != 3:
    print('Wrong number of args')
    exit()
in_file = open(sys.argv[1], 'r')
objects = in_file.read().split('\n\n')

messages = [Message.parse(obj) for obj in objects if obj.startswith('message')]
rpcs = [Rpc.parse(obj) for obj in objects if obj.startswith('rpc')]
oneways = [Oneway.parse(obj) for obj in objects if obj.startswith('oneway')]

message_enums = ['\t' + (m.name + " = 1" if i == 0 else m.name)
                for i, m in enumerate(messages)]
rpc_enums = ['\t' + (rpc.name + " = 1" if i == 0 else rpc.name)
             for i, rpc in enumerate(rpcs)]
oneway_enums = ['\tOneway' + ow.name for i, ow in enumerate(oneways)]
message_text = ""
for msg in messages:
    members = [member_template.substitute(
        {'type': var[1], 'member_name': var[0], 'default': get_default(var[1])}) for var in msg.members]
    appends = [append_template.substitute(
        {'member': var[0]}) for var in msg.members]
    reads = [read_template.substitute({'member': var[0]})
             for var in msg.members]

    message_text += message_template.substitute(
        {'name': msg.name, 'members': "\n".join(members), 'appends': "\n".join(appends), 'reads': "\n".join(reads)})

messages_text = message_file_template.substitute(
    {'messages': message_text, 'message_enum_values': ",\n".join(message_enums),  'rpc_enum_values': ",\n".join(rpc_enums + oneway_enums)})


client_rpc_handler_decls = [client_rpc_handler_decl_template.substitute(
    {'name': ow.name, 'req': ow.req_type})
    for ow in oneways]
client_rpc_cases = [client_rpc_case_template.substitute(
    {'name': ow.name, 'req': ow.req_type})
    for ow in oneways]
client_callable_rpc_decls = [client_callable_rpc_template.substitute(
    {'name': rpc.name, 'req': rpc.req_type, 'resp': rpc.resp_type})
    for rpc in rpcs]
client_callable_rpc_defs = [client_callable_rpc_def_template.substitute(
    {'client_name': 'RpcServer', 'name': rpc.name, 'req': rpc.req_type, 'resp': rpc.resp_type})
    for rpc in rpcs]
client_text = client_file_template.substitute({
    'rpc_handler_decls': "\n".join(client_rpc_handler_decls),
    'handle_rpc_cases': "\n".join(client_rpc_cases),
    'callable_rpc_decls': "".join(client_callable_rpc_decls),
    'callable_rpc_defs': "\n".join(client_callable_rpc_defs),
})


server_rpc_handler_decls = [server_rpc_handler_decl_template.substitute(
    {'name': rpc.name, 'req': rpc.req_type, 'resp': rpc.resp_type})
    for rpc in rpcs]
server_rpc_cases = [server_rpc_case_template.substitute(
    {'name': rpc.name, 'req': rpc.req_type, 'resp': rpc.resp_type})
    for rpc in rpcs]
server_callable_rpc_decls = [server_callable_rpc_template.substitute(
    {'name': ow.name, 'req': ow.req_type})
    for ow in oneways]
server_callable_rpc_defs = [server_callable_rpc_def_template.substitute(
    {'name': ow.name, 'req': ow.req_type})
    for ow in oneways]
server_text = server_file_template.substitute({
    'rpc_handler_decls': "\n".join(server_rpc_handler_decls),
    'handle_rpc_cases': "\n".join(server_rpc_cases),
    'callable_rpc_decls': "".join(server_callable_rpc_decls),
    'callable_rpc_defs': "\n".join(server_callable_rpc_defs),
})

messages_file = open(sys.argv[2] + '/generated_messages.hpp', 'w+')
messages_file.write(messages_text)
messages_file.close()

server_file = open(sys.argv[2] + '/generated_rpc_server.hpp', 'w+')
server_file.write(server_text)
server_file.close()

client_file = open(sys.argv[2] + '/generated_rpc_client.hpp', 'w+')
client_file.write(client_text)
client_file.close()
