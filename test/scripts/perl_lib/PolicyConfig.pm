#!/usr/bin/perl

package PolicyConfig;
# use Data::Dumper;
use strict;

sub new {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self  = {};
    $self->{CONFIG_LOCK_NM}  = undef;
    $self->{CONFIG_BLOCK_LN} = [];
    $self->{STATES} = 0;
    $self->{DEF_AUZN} = 'allow-cfg';
    bless ($self, $class);
    return $self;
}

sub build_config_line
{
    my $self = shift;
    my $name = shift;
    my $value = shift;

    my $line = qq(  <Config name="$name" value="$value" />);
    # print "Adding $line \n";

    # push @{$self->{CONFIG_BLOCK_LN}}, $line if $value ne "";
    push @{$self->{CONFIG_BLOCK_LN}}, $line if $name ne "name";
}

sub build_config_lines
{
    my $self = shift;
    my $array = shift;

    foreach my $name (keys %{$array})
    {
    	# print "Building line for $name \n";
	$self->build_config_line($name, $$array{$name});
    }
}

sub save_config
{
    my $self = shift;
    my $title = shift;

    my $cfg_name = $self->{CONFIG_BLOCK_NM};
    my $blob;

    if ($title) {
	$blob = qq(<$cfg_name name="$title">) . "\n";
    }
    else {
	$blob = qq(<$cfg_name>) . "\n";
    }

    foreach my $line (@{$self->{CONFIG_BLOCK_LN}}) {
	# print "saving lines $line \n";
	$blob .= $line . "\n";
    }

    $blob .= qq(</$cfg_name>) . "\n";

    push  @{$self->{CONFIGS}}, $blob;
    undef $self->{CONFIG_BLOCK_NM};
    undef $self->{CONFIG_BLOCK_LN};
}

sub set_config_block 
{
    my $self = shift;
    my $name = shift;
    $self->{CONFIG_BLOCK_NM} = $name;
}

sub LDAP_AUZN
{
    my $self  = shift;
    my %hash  = @_;
    my $hasht = \%hash;
    my $name  = $hasht->{name};
    undef $hash{name};

    my $LDAP_STATE = 0x0001;
    my $LDAP_AUZN_STATE = 0x0002;

    if (($self->{STATES} & $LDAP_STATE) == 0) {
	$self->LDAP(enabled => 0, name => '_boot_ldap');
    }

    if (($self->{STATES} & $LDAP_AUZN_STATE) == 0) 
    {
	$self->{STATES} = $self->{STATES} | $LDAP_STATE;

	$self->set_config_block ("Auzn-Service-LDAP");
	$self->build_config_line  ("enabled", "1");
	$self->save_config ();
    }

    $self->check_LDAP_parameters ($hasht);
    $self->set_config_block ("Auzn-Handle-LDAP");
    $self->build_config_lines ($hasht);
    $self->save_config ("$name");
}

sub LDAP
{
    my $self = shift;
    my %hash  = @_;
    my $hasht = \%hash;

    my $name  = $hasht->{name};
    my $LDAP_STATE = 0x0001;
    undef $hash{name};

    if (($self->{STATES} & $LDAP_STATE) == 0) {
	$self->{STATES} = $self->{STATES} | $LDAP_STATE;

	$self->set_config_block ("Auth-Service-LDAP");
#fix       "number-ldap-threads", 
#fix       "library", 
#fix       "library-prefix", 
	$self->build_config_line  ("enabled", "1");
	$self->save_config ();
    }

    $self->check_LDAP_parameters ($hasht);

    $self->set_config_block ("Auth-Handle-LDAP");
    $self->build_config_lines ($hasht);
    $self->save_config ("$name");
}

sub check_LDAP_parameters
{
    my $self = shift;
    my $hasht = shift;

    my @required = ("enabled", 
		    "server-name", 
		    "base-dn", 
		    "uid-filter"
		    );

    my @optional = ("bind-dn", 
		    "bind-pwd", 
		    "bind-pwd-file",
		    "server-port", 
		    "use-attributes", 
		    "attribute-name", 
		    "attribute-value", 
		    "secure-bind", 
		    "cert-db-location", 
		    "query-timeout", 
		    "canonical-name", 
		    "cache-ttl-minutes");

    $self->check_generic_parameters ("LDAP", $hasht, 
				     \@required, \@optional);
}

sub check_generic_parameters
{
    my $self = shift;
    my $name = shift;
    my $hasht = shift;
    my $required = shift;
    my $optional = shift;

    my $count = 0;
    my $found = 0;
    my $key;

    foreach $key (keys %$hasht) {
	$found = 0;

	if (grep {/$key/} @$required) {
	    $found = 1;
	    $count ++;
	}

	if ($found == 0 && 
	    $key ne "name" &&
	    ! grep {/$key/} @$optional) 
	{
	    $self->warn("minor", 
			"Unable to verify $name attribute '$key'");
	}
    }

    if ($count < $#$required) {
	my $tmp = $#$required;
	$self->warn("major", 
		    "One or more required $name attributes not present\n"
		    . "Found only $count elments out of $tmp");

    }
}

sub warn {
    my $self = shift;
    my $level = shift;

    if    ($level eq "minor") { print "Warning: ", @_, "\n"; }
    elsif ($level eq "major") { print "Error: ", @_, "\n"; }
    else  { print "Fatal: ", @_, "\n"; exit; }
}

sub NTLM
{
    my $self = shift;
    my %hash = @_;
    my $name = $hash{name};
    undef $hash{name};
    my $hasht = \%hash;

    # print Dumper($hasht);

    $self->set_config_block ("Auth-Handle-NTLM");
    $self->check_NTLM_parameters ($hasht);
    $self->build_config_lines ($hasht);
    $self->save_config ("$name");
}

sub check_NTLM_parameters
{
    my $self = shift;
    my $hasht = shift;

    my @required = ("enabled", 
		    "dc-list", 
		    "nt-domain", 
		    "nt-host"
		    );

    my @optional = ("dc-load-balance", 
		    "dc-max-connections", 
		    "dc-max-conn-time", 
		    "queue-len", 
		    "req-timeout",
		    "dc-retry-time", 
		    "dc-fail-threshold",
		    "fail-open", 
		    "allow-guest-login"
		    );

    $self->check_generic_parameters ("NTLM", $hasht, 
				     \@required, \@optional);
}

sub RADIUS
{
    my $self = shift;
    my %hash = @_;
    my $hasht = \%hash;
    my $name = $hasht->{name};
    undef $hash{name};

    $self->set_config_block ("Auth-Handle-RADIUS");
    $self->check_RADIUS_parameters ($hasht);
    $self->build_config_lines ($hasht);
    $self->save_config ("$name");
}

sub check_RADIUS_parameters
{
    my $self = shift;
    my $hasht = shift;

    my @required = ( "primary-server-name", 
		     "primary-server-shared-key",
		     "enabled"
		    );

    my @optional = ( "primary-server-auth-port",
		     "primary-server-acct-port",
		     "secondary-server-name",
		     "secondary-server-auth-port",
		     "secondary-server-acct-port",
		     "secondary-server-shared-key",
		     "timeout-interval",
		     "max-retries",
		     "ttl",
		     "purge-cache-on-auth-failure",
		     );

    $self->check_generic_parameters ( "RADIUS", $hasht, 
				      \@required, \@optional);
}

sub write
{
    my $self = shift;
    my $file_name = shift;
    my $fref;

    open ($fref, "> $file_name") 
	|| $self->warn ("major", "unable to write $file_name");

    my $text = $self->gen_config();
    print $fref $text;
    close ($fref);

}

sub dump
{
    my $self = shift;
    my $text = $self->gen_config();
    print "Dumping Configuration: \n";
    print $text;
}

sub config
{
    my $self = shift;
    my $header = "[policy_config.xml]\n";
    my $text = $self->gen_config();
    return $header . $text;
}

sub gen_config
{
    my $self = shift;
    my $txt  = "";

    $txt .= qq{<?xml version="1.0"?>\n<Policy>\n<Configuration>\n};
    $txt .= qq{<Auzn-Handle-STATIC name="allow-cfg" provider="allow"/>\n};
    $txt .= qq{<Auzn-Handle-STATIC name="deny-cfg" provider="deny"/>\n};
    $txt .= qq{<Auth-Handle-STATIC name="dummy-cfg" provider="none">\n};
    $txt .= qq{   <Config name="dummy-provider" value="1"/>\n};
    $txt .= qq{</Auth-Handle-STATIC>\n};

    foreach my $cfg (@{$self->{CONFIGS}}) {
	$txt .= $cfg . "\n";
    }

    $txt .= qq{</Configuration>\n<Acl>\n};

    foreach my $key (@{$self->{KEYS}}) {
	$txt .= "\n" . $key;
    }

    foreach my $ruleset (@{$self->{ACL_SETS}}) {
	$txt .= $ruleset;
    }

    $txt .= qq{</Acl>\n</Policy>\n};
    return $txt;
}

sub KEY {
    my $self = shift;
    my $keyId  = shift;
    my $criteria;

    my $new_key .= qq(<Key keyId="$keyId">\n);
    foreach $criteria (@_) {
	$new_key .= "  " . $criteria;
    }
    $new_key .= qq(</Key>\n);

    push @{$self->{KEYS}}, $new_key;
}

sub CRITERIA {
    my $self = shift;
    my %hash = @_;
    my ($method, $type, $value, $name) = (
					  $hash{method},
					  $hash{type},
					  $hash{value},
					  $hash{name});
    my $res;
    $self->check_CRITERIA ($method, $type, $value, $name);
    $res  = qq(<Criteria method="$method" type="$type" value="$value");
    $res .= qq( name="$name") if $name;
    $res .= qq(/>\n);

    return $res;
}

sub check_CRITERIA {
    my $self = shift;
    my ($method, $type, $value, $name) = @_;

    my %type_map = 
      ( 
	"dest_domain" => [ "domain", "exact" ],
	"dest_port"   => [ "exact", "range"],
	"dest_ip"     => [ "exact", "range", "mask"],
	"host"        => [ "host"],
	"time"        => [ "range"],
	"key"         => [ "exact", "regex" ],
	"url"         => [ "exact", "regex" ],
	"suffix"      => [ "suffix" ],
	"prefix"      => [ "prefix" ],
	);

    if (! defined $type_map{$type}) {
	$self->warn ("minor",
		     "Specifying unknown search criteria type '$type'");
    }
    else {
	my $arr = $type_map{$type};

	if (! grep {/$method/} @$arr) {
	    $self->warn ("minor",
		 "Specifying unknown search criteria method '$method'");
	}
    }

    if ($method eq "key" && "X$name" eq "X") {
	$self->warn ("major",
		     "Search criteria 'key' needs a 'name' parameter");
    } 

    if ("X$value" eq "X") {
	$self->warn ("major",
		     "Search criteria needs a 'value' parameter");
    }
};

sub ACL {
    my $self = shift;
    my $scope  = shift;
    my $rule;

    my $new_key .= qq(<RuleSet scope="$scope">\n);
    foreach $rule (@_) {
	$new_key .= $rule;
    }

    if ($scope eq 'TE') {
       my $da = $self->{DEF_AUZN};
       $new_key .= qq(   <DefaultRule authorizor="$da"/>\n);
    }

    $new_key .= qq(</RuleSet>\n);

    push @{$self->{KEYS}}, $new_key;
}

sub DEFAULT_DENY {
    my $self = shift;
    $self->{DEF_AUZN} = 'deny-cfg';
    $self->ACL ("TE");
}

sub DEFAULT_ALLOW {
    my $self = shift;
    $self->{DEF_AUZN} = 'allow-cfg';
    $self->ACL ("TE");
}
    

sub ACL_ALL {
    my $self = shift;

    $self->KEY ( "acl_all_key" );
#	$self->CRITERIA ( 
#	   type   => 'dest_domain',
#	   method => 'domain',
#	   value  => '.' ));

    $self->ACL ("TE", $self->RULE(keyId => 'acl_all_key', @_));
}

sub RULE 
{
    my $self = shift;
    my %hash = @_;
    my $keyId = $hash{keyId};
    my $auth = $hash{auth};
    my $auzn = $hash{auzn};
    my $ruleData = $hash{ruleData};

    my $rule;
    $rule  = qq(  <Rule keyId="$keyId" );
    $rule .= qq( authenticator="$auth") if $auth;
    $rule .= qq( authorizor="$auzn") if $auzn;

    if ($ruleData && $#$ruleData >= 0) {
	$rule .= qq(>\n);
	my $elem;
	foreach $elem (@$ruleData) {
	    $rule .= qq(    $elem\n);
	}
	$rule .= qq(  </Rule>\n);
    }
    else {
	$rule .= qq(/>\n);
    }
}

sub RULE_DATA 
{
    my $self = shift;
    my $arr;
    my @ret = ();
    my $nm;
    my $vl;

    foreach $arr (@_) {
	$nm = $arr->[0];
	$vl = $arr->[1];
	push @ret, qq(<RuleData name="$nm" value="$vl" />);
    }

    return \@ret;
}

1;

__END__

use PolicyConfig;
# use Data::Dumper;

my $config = new POLICY_CONFIG;

$config->LDAP (
  'name' => "ldap-1", 
  'enabled' => 1,
  'base-dn' => "ou=people,dc=inktomi,dc=com",
  'uid-filter' => "uid",
  'server-name' => "ldap.inktomi.com",
);

$config->LDAP (
  'name' => "ldap-2", 
  'base-dn' => "ou=people,dc=inktomi,dc=com",
  'uid-filter' => "uid",
  'server-name' => "ldap.inktomi.com",
  'chump' => "three",
);

$config->LDAP_AUZN (
  'name' => "ldap-3", 
  'base-dn' => "ou=people,dc=inktomi,dc=com",
  'uid-filter' => "uid",
  'server-name' => "ldap.inktomi.com",
  'chump' => "three",
);

$config->NTLM (
  'name'      => "ntlm-1",
  'dc-list'   => "localhost:15501",
  'enabled'   => "1",
  'nt-domain' => "TSQA",
  'nt-host'   => "TSQA",
);

$config->RADIUS (
  'name' => "radius-1",
  'primary-server-name' => "radius.qa.inktomi.com",
  'primary-server-shared-key' => "SOME_PASSWORD",
);

$config->KEY (
	      "key1",
	      $config->CRITERIA ( 
		      type   => dest_domain,
		      method => domain,
		      value  => inktomi.com ),
	      $config->CRITERIA ( 
		      type   => dest_port,
		      method => range,
		      value  => 25-30 ),
	      );

$config->KEY (
	      "key2",
	      $config->CRITERIA ( 
		      type   => dest_domain,
		      method => domain,
		      value  => '.' ),
	      );

my $rule_data = $config->RULE_DATA 
    (
     [ "some_name", "some_value" ],
     [ "some_name2", "some_value2" ],
     [ "some_name3", "some_value3" ],
     );

$config->ACL (
	"TE:http",
	$config->RULE ( 
		keyId => "key1", 
		auth  => "ldap-1",
		auzn  => "allow-cfg",
		ruleData => $rule_data,
		),
	$config->RULE ( 
		keyId => "key2", 
		auzn  => "deny-cfg",
		),
	);

$config->ACL (
	"TE:http",
	$config->RULE ( 
		keyId => "key1", 
		auzn  => "deny",
		),
	);

# print "This is the pending config:\n";
# print Dumper($config);

$config->dump();
$config->write('test.xml');
