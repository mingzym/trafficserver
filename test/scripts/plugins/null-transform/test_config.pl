use ConfigHelper;

my $cfg = new ConfigHelper;

# edit plugins.config
$cfg->add_config('plugins.config');
$cfg->add_config_line ('vscan.so');

# edit filters.config
$cfg->add_config('filter.config');
$cfg->add_config_line ('dest_domain=. action=ldap');

# change records.config
$cfg->set_record ('proxy.config.cache.storage_filename', 'storage.new');
$cfg->add_record ('INT', 'dummy.record', '12');

#print
print $cfg->output;

my $rec = $cfg->records_config;
   $rec .= <<EOF;
add CONFIG blah int 0
EOF
print $rec;
