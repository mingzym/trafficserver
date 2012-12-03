use PolicyConfig;
# use Data::Dumper;

my $pcfg = new PolicyConfig;
my $pcfg2 = new PolicyConfig;

$pcfg->LDAP (
  'name' => "ldap-1",
  'enabled' => 1,
  'base-dn' => "dc=qa,dc=wumpus,dc=org",
  'attribute-name' => 'userGroup',
  'attribute-value' => '0',
  'server-name' => "othello.climate.inktomi.com",
  'uid-filter' => "uid",
);

$pcfg->KEY ("all",
            $pcfg->CRITERIA (
              type => dest_domain,
              method => domain,
              value  => '.'));

$pcfg->ACL ("TE:http",
              auth  => "ldap-1");

my $ts_config = $pcfg->config;

print $ts_config;

$pcfg2->LDAP (
  'name' => "ldap-1",
  'enabled' => 1,
  'base-dn' => "dc=qa,dc=wumpus,dc=org",
  'attribute-name' => 'userGroup',
  'attribute-value' => '0',
  'server-name' => "othello.climate.inktomi.com",
  'uid-filter' => "uid",
);

$pcfg2->ACL_ALL (auth => "ldap-1");

print $pcfg2->config;
