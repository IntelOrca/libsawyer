function getInformation() {
    return new Promise<string>((resolve, reject) => {
        resolve('test');
    });
}

async function go() {
    let info = await getInformation();
    finish(info);
}

go();
