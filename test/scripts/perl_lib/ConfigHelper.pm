#
#  ConfigHelper.pm
#
#  Aides in generating some config elements.
#
# Author: bevans
#

# (todo: modify so it can read input streams.)

package ConfigHelper;
# use PolicyConfig;
use strict;

sub new {
   my $proto = shift;
   my $class = ref ($proto) || $proto;
   my $self = {};
   my %HASH = (@_);

   bless($self, $class);

   $self->defaults();

   if ( ! $HASH{'SkipPolicyGeneration'} ) {
       $self->add_config ('policy_config.xml');
       my $pcfg = new PolicyConfig;
       $pcfg->DEFAULT_ALLOW;
       $self->add_config_line ($pcfg->gen_config);
   }

   return $self;
}

sub set_storage
{
   my $self = shift;
   my $amount = shift;

   $self->{STORAGE_SIZE} = $amount;
}

sub set_hostdb
{
   my $self = shift;
   my $amount = shift;

   $self->set_record ('proxy.config.hostdb.size', $amount);
}

sub my_debug
{
   my @vals = @_;
   # print @vals;
}

sub set_record
{
   my $self = shift;
   my $rname = shift;
   my $rval  = shift;

   my_debug "Setting record $rname $rval\n";
   $self->{RECORDS_CONFIG_SUB}->{$rname} = $rval;
}

sub add_record
{
   my $self = shift;
   my $type  = shift;
   my $rname = shift;
   my $rval  = shift;

   my_debug "Adding record $rname $type $rval\n";
   push @{$self->{RECORDS_CONFIG_ADD}}, [ $type, $rname, $rval ];
}

sub add_record_i
{
   my $self = shift;
   my $rname = shift;
   my $rval  = shift;

   $self->add_record ('INT', $rname, $rval);
}

sub add_record_s
{
   my $self = shift;
   my $rname = shift;
   my $rval  = shift;

   $self->add_record ('STRING', $rname, $rval);
}

sub set_debug_tags
{
   my $self = shift;
   my $tags = shift;

   $self->add_record_i ('proxy.config.diags.debug.enabled', '1');
   $self->add_record_s ('proxy.config.diags.debug.tags', $tags);
}

sub set_action_tags
{
   my $self = shift;
   my $tags = shift;

   $self->add_record_i ('proxy.config.diags.action.enabled', '1');
   $self->add_record_s ('proxy.config.diags.action.tags', $tags);
}

sub defaults
{
   my $self = shift;

   $self->set_record ('proxy.config.proxy_name',  'deft.inktomi.com');
   $self->set_record ('proxy.config.hostdb.size', '50000');
   $self->set_record ('proxy.config.cache.storage_filename', 'storage.config');
   $self->set_record ('proxy.config.cop.core_signal', 0);

   $self->add_record_i ('proxy.config.core_limit', -1);
   $self->add_record_i ('proxy.config.dump_mem_info_frequency', 0);
   $self->add_record_i ('proxy.config.http_ui_enabled', 1);
   $self->add_record_i ('proxy.config.http.insert_request_via_str', 1);
   $self->add_record_i ('proxy.config.http.insert_response_via_str', 1);
   $self->add_record_i ('proxy.config.http.send_http11_requests', 2);

   $self->add_record_s ('proxy.config.diags.output.debug', 'LO');
   $self->add_record_s ('proxy.config.diags.output.diag', 'LO');
   $self->add_record_s ('proxy.config.diags.output.status', 'O');
   $self->add_record_s ('proxy.config.diags.output.note', 'O');
   $self->add_record_s ('proxy.config.diags.output.warning', 'O');
   $self->add_record_s ('proxy.config.diags.output.error', 'E');
   $self->add_record_s ('proxy.config.diags.output.fatal', 'E'); 
   $self->add_record_s ('proxy.config.diags.output.alert', 'E');
   $self->add_record_s ('proxy.config.diags.output.emergency', 'E');

   #
   # $self->add_record_i ('proxy.config.diags.debug.enabled', '1');
   # $self->add_record_s ('proxy.config.diags.debug.tags', 'NULL');
   $self->add_record_i ('proxy.config.diags.action.enabled', '1');
   $self->add_record_s ('proxy.config.diags.action.tags', 'deft.*');

   $self->set_storage ('52428800');
}

sub records_config
{
   my $self = shift;
   my $output = "[records.config]\n";
   my ($hr, $ar, $aar);

   # replacements
   $hr = $self->{RECORDS_CONFIG_SUB};
   foreach my $key (keys %$hr) {
      my $_v = $hr->{$key};
      $output .= "$key $_v\n";
   }

   # additions
   $ar = $self->{RECORDS_CONFIG_ADD};
   foreach $aar (@$ar) {
      my $_t = $aar->[0];
      my $_n = $aar->[1];
      my $_v = $aar->[2];
      $output .= "add CONFIG $_n $_t $_v\n";
   }

   return $output;
}

sub output 
{
   my $self = shift;
   my $output;
   my $cfg;

   $output .= $self->records_config();
   $output .= $self->storage_config();

   foreach $cfg (@{$self->{CONFIGS}}) {
     $output .= $self->print_config ($cfg);
   }

   return $output;
}

sub storage_config
{
   my $self = shift;
   my $output = "[storage.config]\n";
   $output .= ".  " . $self->{STORAGE_SIZE} . "\n";

   return $output;
}

sub print_config 
{
   my $self = shift;
   my $cfg_name = shift;
   my $cfg = "CF_" . $cfg_name;
   my $line;
  
   my $output = "[$cfg_name]\n";
   foreach $line (@{$self->{$cfg}}) {
      $output .= $line . "\n";
   }

   return $output;
}

sub add_config 
{
   my $self = shift;
   my $name = shift;
   push @{$self->{CONFIGS}}, $name;
   $self->{CUR_CFG} =  $name;
}

sub add_config_line
{
   my $self = shift;
   my $line = shift;

   my $cfg = "CF_" . $self->{CUR_CFG};
   push @{$self->{$cfg}}, $line;
}

1;

__END__

use ConfigHelper;

my $cfg = new ConfigHelper;

$cfg->set_debug_tags('http_hdrs|ldap.*');
$cfg->add_config('plugins.config');
$cfg->add_config_line ('vscan.so');

$cfg->add_config('storage.new');
$cfg->add_config_line('. 120000000');

$cfg->add_config('filter.config');
$cfg->add_config_line ('dest_domain=. action=ldap');

# change records.config
$cfg->set_record ('proxy.config.cache.storage_filename', 'storage.new');
$cfg->add_record ('INT', 'dummy.record', '12');

#print
print $cfg->output;
