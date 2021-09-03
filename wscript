# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('vanetsim', [
                                   'core',
                                   'network',
                                   'mobility',
                                   'wave',
                                   'csma',
                                   'internet',
                                   'point-to-point',
                                   'applications',
                                   'traci',])
                                   
    module.source = [
    ]

    headers = bld(features='ns3header')
    headers.module = 'vanetsim'
    headers.source = [
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
