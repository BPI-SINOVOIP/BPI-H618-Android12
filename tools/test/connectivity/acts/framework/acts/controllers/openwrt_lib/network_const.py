# params for ipsec.conf
IPSEC_CONF = {
    "config setup": {
        "charondebug": "chd 2,ike 2,knl 2,net 2,esp 2,dmn 2,"
                       "mgr 2,lib 1,cfg 2,enc 1".__repr__(),
        "uniqueids": "never"
    },
    "conn %default": {
        "ike": "aes128-sha-modp1024",
        "esp": "aes128-sha1"
    }
}

IPSEC_L2TP_PSK = {
    "conn L2TP_PSK": {
        "keyexchange": "ikev1",
        "type": "transport",
        "left": "192.168.1.1",
        "leftprotoport": "17/1701",
        "leftauth": "psk",
        "right": "%any",
        "rightprotoport": "17/%any",
        "rightsubnet": "0.0.0.0/0",
        "rightauth": "psk",
        "auto": "add"
    }
}

IPSEC_L2TP_RSA = {
    "conn L2TP_RSA": {
        "keyexchange": "ikev1",
        "type": "transport",
        "left": "192.168.1.1",
        "leftprotoport": "17/1701",
        "leftauth": "pubkey",
        "leftcert": "serverCert.der",
        "right": "%any",
        "rightprotoport": "17/%any",
        "rightsubnet": "0.0.0.0/0",
        "rightauth": "pubkey",
        "auto": "add"
    }
}

# parmas for lx2tpd

XL2TPD_CONF_GLOBAL = [
    "[global]",
    "ipsec saref = no",
    "debug tunnel = no",
    "debug avp = no",
    "debug network = no",
    "debug state = no",
    "access control = no",
    "rand source = dev",
    "port = 1701",
]

XL2TPD_CONF_INS = [
    "[lns default]",
    "require authentication = yes",
    "pass peer = yes",
    "ppp debug = no",
    "length bit = yes",
    "refuse pap = yes",
    "refuse chap = yes",
]

XL2TPD_OPTION = [
    "require-mschap-v2",
    "refuse-mschap",
    "ms-dns 8.8.8.8",
    "ms-dns 8.8.4.4",
    "asyncmap 0",
    "auth",
    "crtscts",
    "idle 1800",
    "mtu 1410",
    "mru 1410",
    "connect-delay 5000",
    "lock",
    "hide-password",
    "local",
    "debug",
    "modem",
    "proxyarp",
    "lcp-echo-interval 30",
    "lcp-echo-failure 4",
    "nomppe"
]

# iptable rules for vpn_pptp
FIREWALL_RULES_FOR_PPTP = [
    "iptables -A input_rule -i ppp+ -j ACCEPT",
    "iptables -A output_rule -o ppp+ -j ACCEPT",
    "iptables -A forwarding_rule -i ppp+ -j ACCEPT"
]

# iptable rules for vpn_l2tp
FIREWALL_RULES_FOR_L2TP = [
    "iptables -I INPUT  -m policy --dir in --pol ipsec --proto esp -j ACCEPT",
    "iptables -I FORWARD  -m policy --dir in --pol ipsec --proto esp -j ACCEPT",
    "iptables -I FORWARD  -m policy --dir out --pol ipsec --proto esp -j ACCEPT",
    "iptables -I OUTPUT   -m policy --dir out --pol ipsec --proto esp -j ACCEPT",
    "iptables -t nat -I POSTROUTING -m policy --pol ipsec --dir out -j ACCEPT",
    "iptables -A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT",
    "iptables -A INPUT -p esp -j ACCEPT",
    "iptables -A INPUT -i eth0.2 -p udp --dport 500 -j ACCEPT",
    "iptables -A INPUT -i eth0.2 -p tcp --dport 500 -j ACCEPT",
    "iptables -A INPUT -i eth0.2 -p udp --dport 4500 -j ACCEPT",
    "iptables -A INPUT -p udp --dport 500 -j ACCEPT",
    "iptables -A INPUT -p udp --dport 4500 -j ACCEPT",
    "iptables -A INPUT -p udp -m policy --dir in --pol ipsec -m udp --dport 1701 -j ACCEPT"
]


# Object for vpn profile
class VpnL2tp(object):
    """Profile for vpn l2tp type.

    Attributes:
        hostname: vpn server domain name
        address: vpn server address
        username: vpn user account
        password: vpn user password
        psk_secret: psk for ipsec
        name: vpn server name for register in OpenWrt
    """

    def __init__(self,
                 vpn_server_hostname,
                 vpn_server_address,
                 vpn_username,
                 vpn_password,
                 psk_secret,
                 server_name):
        self.name = server_name
        self.hostname = vpn_server_hostname
        self.address = vpn_server_address
        self.username = vpn_username
        self.password = vpn_password
        self.psk_secret = psk_secret
