# @file        index.py
#              PythonMonkey loader for the dcp-client package.
#
#              Question: should this be a PythonMonkey .py CommonJS Module, or a Python module?
#                        It is current a CJS module.
#
#              During module initialization, we load dist/dcp-client-bundle.js from the
#              same directory as this file, and setup a minimum compatibility environment
#              for dcp-client to execute in.
#
#              Note that while this code may use PythonMonkey's require() to get things done,
#              require is not exposed as a symbol to JS here; the environment used by dcp-client
#              for PythonMonkey looks pretty much like a web browser to keep things like SocketIO
#              happy.
#
#              A few OS-level interfaces are plumbed in, however, so that the dcp-client UX
#              remains somewhat similar to Node.js.  In particular,
#              - window.localStorage is on disk and compatible with the Node.js version
#              - id and bank keystores are loaded from the same location on disk as the Node.js version
#              - FUTURE: support for oAuth
#              - FUTURE: support for dcp-config fragments based on the Node.js loader (./index.js)
#              - FUTURE: support for bundle auto-update similar to the Node.js loader
#
# @author      Will Pringle, will@distributive.network
# @author      Wes Garland, wes@distributive.network
# @date        Feb 2024
#
import pythonmonkey as pm
import os
import urllib.request
import asyncio

dcp_client_bundle_filename = os.path.dirname(__file__) + '/dcp-client-bundle.js'
dcp_support = pm.require('./dcp-support');
fs_basic    = pm.require('./fs-basic');

# load dcp-client, then run the callback function
async def load_dcp_client():
    cb_retval = None
    try:
        dcp_config_js = urllib.request.urlopen('https://scheduler.distributed.computer/etc/dcp-config.js').read().decode();
        pm.eval('globalThis.window = {}; globalThis.dcpConfig =' + dcp_config_js);
        pm.eval('delete globalThis.window') # we might need to keep window?
        pm.eval('globalThis.dcpConfig.build = "debug";'); # we should fix the bundle so that this is not necessary
        pm.eval('globalThis.crypto = {};');
        pm.globalThis['crypto']['getRandomValues'] = dcp_support['getRandomValues']

        bundle_code = fs_basic['readFile'](dcp_client_bundle_filename)
        dcp_client_modules = pm.eval(bundle_code)
        pm.eval('globalThis')['dcp'] = dcp_client_modules;
        pm.eval('Object.assign')(pm.globalThis.dcp['fs-basic'], fs_basic);
        dcp_client_modules['utils']['expandPath'] = os.path.expanduser;
    

        pm.eval(''''use strict';
const job = dcp.compute.for(Array.from('hello world'), function workFn(letter) {
  progress();
  return letter.toUpperCase();  
});
''');
        print("about to eval after compute.for")

        pm.eval('''
job.on('readystatechange', (state) => console.log(`readystatechange: ${state}`));
job.on('submit', console.log);
job.on('accepted', console.log);
job.on('result', console.log);
job.on('console', console.log);
job.on('error', console.log);     
''')
        
        print("about to job deploy")

        job_deploy = pm.eval('''
async function jobDeploy()
{
    console.log("jobDeploy start");                    

    const theKeystore = await new dcp.wallet.Keystore('{"address":"0x6db720Ec2e7DB70737Cf0CC6986711e653a25E71","crypto":{"ciphertext":"43a8dc28fd6c3a571ec719b6ca86100ccd5e128431fc34dbc1d80875b3bf7aa2","cipherparams":{"iv":"2cbd4e1db754b4839741138d4b33c5a1"},"cipher"    :"aes-128-ctr","kdf":"scrypt","kdfparams":{"dklen":32,"salt":"be5a8db9714f86146f4f03322d43b182c4faaba1cd95680ffca94ea5b6648419","n":1024,"r":8,"p":1},"mac":"9e4ffb60a33d57db3d25e6b8077afd53a8b194bb68c6b38b90205c53c4d62470"},"version":3}');

    const theIdKeystore = await new dcp.wallet.IdKeystore('{"address":"0x6db720Ec2e7DB70737Cf0CC6986711e653a25E71","crypto":{"ciphertext":"43a8dc28fd6c3a571ec719b6ca86100ccd5e128431fc34dbc1d80875b3bf7aa2","cipherparams":{"iv":"2cbd4e1db754b4839741138d4b33c5a1"},"cipher"    :"aes-128-ctr","kdf":"scrypt","kdfparams":{"dklen":32,"salt":"be5a8db9714f86146f4f03322d43b182c4faaba1cd95680ffca94ea5b6648419","n":1024,"r":8,"p":1},"mac":"9e4ffb60a33d57db3d25e6b8077afd53a8b194bb68c6b38b90205c53c4d62470"},"version":3}');


    job.setPaymentAccountKeystore(theKeystore);
    dcp.wallet.addId(theIdKeystore);

    console.log("jobDeploy about to exec");                      
        
    return job.exec() //.then(results => console.log('wtf', results));
}
jobDeploy''')
        my_ret = await job_deploy()

        print("jobDeploy done")  


        #if (callback):
        #    cb_retval = callback()
    except Exception as error:
        print('Error loading bundle:', error)
    await pm.wait() # blocks until all asynchronous calls finish
    return cb_retval

def init():
    asyncio.run(load_dcp_client(), debug=True)
    
# exports of dcp-client python-language CommonJS module
#exports['init'] = init;
    
init()    