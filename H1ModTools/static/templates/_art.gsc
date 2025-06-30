// _createart generated.  modify at your own risk. Changing values should be fine.
main()
{
    level.tweakfile = 1;

    if ( isusinghdr() )
        maps\createart\mapname_fog_hdr::main();
    else
        maps\createart\mapname_fog::main();

    visionsetnaked( "mapname", 0 );
}