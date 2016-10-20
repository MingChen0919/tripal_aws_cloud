<?php

## Create ec2 client
use Aws\Credentials\CredentialProvider;
use Aws\Ec2\Ec2Client;

$provider = CredentialProvider::ini();

$ec2 = new Aws\Ec2\Ec2Client([
          'version' => 'latest',
          'region' => 'us-east-1',
          'credentials' => $provider
          ]);


$describeVpcs = $ec2->describeVpcs([]);
$describeVpcsArr = $describeVpcs->toArray();

if(count($describeVpcsArr)> 4){
  break;
} else {
  dpm('less than 5 VPCs');
  $vpc = $ec2->createVpc([
            'CidrBlock' => '10.0.0.0/16',
            ]);
  $vpcArr = $vpc->toArray();
  $VpcId = $vpcArr['Vpc']['VpcId'];

  dpm($VpcId);

  ##==== create internetgateway =========
  $internetGateway = $ec2->createInternetGateway([]);
  $internetGatewayArr = $internetGateway->toArray();
  $gatewayId = $internetGatewayArr['InternetGateway']['InternetGatewayId'];

  # attach internet gateway to the VPC
  $ec2->attachInternetGateway([
            'InternetGatewayId' => $gatewayId,
            'VpcId' => $VpcId,
          ]);

  ##===== Route table ===================
  $routeTable = $ec2->createRouteTable([
            'VpcId' => $VpcId,
          ]);
  $routeTableArr = $routeTable->toArray();
  $routeTableId = $routeTableArr['RouteTable']['RouteTableId'];
  # crate route
  $ec2->createRoute([
            'DestinationCidrBlock' => '0.0.0.0/0',
            'GatewayId' => $gatewayId,
            'RouteTableId' => $routeTableId,  
          ]);

  ##==== create two public subnets ======
  # get available zones
  $availZones = $ec2->describeAvailabilityZones([]);
  $availZonesArr =  $availZones->toArray();
  $availZone1 = $availZonesArr['AvailabilityZones'][0]['ZoneName'];
  $availZone2 = $availZonesArr['AvailabilityZones'][1]['ZoneName'];

  # public subnet 1
  $public_subnet_1 = $ec2->createSubnet([
            'AvailabilityZone' => $availZone1,
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.0.0/24',
            ]);
  $public_subnet_1_Arr = $public_subnet_1->toArray();
  $public_subnet_1_ID = $public_subnet_1_Arr['Subnet']['SubnetId'];

  # associate route table with public subnet 1
  $associateRouteTable = $ec2->associateRouteTable([
            'RouteTableId' => $routeTableId,
            'SubnetId' => $public_subnet_1_ID,
          ]);

  # enable auto-assign public IP (required for public subnet)
  $ec2->modifySubnetAttribute([
            'MapPublicIpOnLaunch' => [
              'Value' => true,
            ],
            'SubnetId' => $public_subnet_1_ID,
          ]);
 
  # public subnet 2
  $public_subnet_2 = $ec2->createSubnet([
            'AvailabilityZone' => $availZone2,
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.1.0/24',
            ]);
  $public_subnet_2_Arr = $public_subnet_2->toArray();
  $public_subnet_2_ID = $public_subnet_2_Arr['Subnet']['SubnetId'];


  # associate route table with public subnet 1
  $associateRouteTable = $ec2->associateRouteTable([
            'RouteTableId' => $routeTableId,
            'SubnetId' => $public_subnet_2_ID,
          ]);
  # enable auto-assign public IP (required for public subnet)
  $ec2->modifySubnetAttribute([
            'MapPublicIpOnLaunch' => [
              'Value' => true,
            ],
            'SubnetId' => $public_subnet_2_ID,
          ]);

  ##========= create two private subnets ==============
  $private_subnet_1 = $ec2->createSubnet([
            'AvailabilityZone' => $availZone1,
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.2.0/24',
            ]);
  $private_subnet_2 = $ec2->createSubnet([
            'AvailabilityZone' => $availZone2,
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.3.0/24',
            ]);

  ##======== create security group for web server =====
  $securityGroupWebServer = $ec2->createSecurityGroup([
            'Description' => 'Security group for web server',
            'GroupName' => 'webServerSecurityGroup',
            'VpcId' => $VpcId,
          ]);
  $securityGroupWebServerArr = $securityGroupWebServer->toArray();
  $webServerSGId = $securityGroupWebServerArr['GroupId'];
  # get web server security group details
  $describeWebServerSG = $ec2->describeSecurityGroups([
            'GroupIds' => [$webServerSGId],
          ]);
  $describeWebServerSGArr = $describeWebServerSG->toArray();
  dpm($describeWebServerSGArr);
  $webServerSGOwnerId = $describeWebServerSGArr['SecurityGroups'][0]['OwnerId'];
  dpm($webServerSGOwnerId);

  //  $webServerSGName = $securityGroupWebServerArr;
  #--------- add rules to web server security group----
  # rule 1: SSH(22)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $webServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 22,
            'ToPort' => 22,
            'CidrIp' => '0.0.0.0/0'
          ]);
  # rule 2: HTTP(80)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $webServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 80,
            'ToPort' => 80,
            'CidrIp' => '0.0.0.0/0'
          ]);
  # rule 3: HTTPS(443)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $webServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 443,
            'ToPort' => 443,
            'CidrIp' => '0.0.0.0/0',
          ]);
  # rule 4: SMTPS(465)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $webServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 465,
            'ToPort' => 465,
            'CidrIp' => '0.0.0.0/0'
          ]);

  ##======== create security group for database server =====
  $securityGroupDatabaseServer = $ec2->createSecurityGroup([
            'Description' => 'Security group for database server',
            'GroupName' => 'databaseServerSecurityGroup',
            'VpcId' => $VpcId,
          ]);
  $securityGroupDatabaseServerArr = $securityGroupDatabaseServer->toArray();
  $databaseServerSGId = $securityGroupDatabaseServerArr['GroupId'];
  #--------- add rules to database server security group----
  # rule 1: PostgreSQL(5432)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $databaseServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 5432,
            'ToPort' => 5432,
            'SourceSecurityGroupName' => $webServerSGOwnerId,
          ]);
  # rule 2: MySQL/Aurora(3306)
  $ec2->authorizeSecurityGroupIngress([
            'GroupId' => $databaseServerSGId,
            'IpProtocol' => 'tcp',
            'FromPort' => 3306,
            'ToPort' => 3306,
            'SourceSecurityGroupName' => $webServerSGOwnerId,
          ]);

}

