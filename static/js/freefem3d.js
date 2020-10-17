//initial for 3D
var frustumSize;
var camera, scene, renderer, mesh, mesh_border, axes, helpers;
var controls;
var clipPlanes;

//output btn for 3D PNG
$('#3dpng').click(function () {
    var canvas = document.querySelector("#new_plot > canvas");
    canvas.getContext("experimental-webgl", { preserveDrawingBuffer: true });
    var imgURI = canvas.toDataURL('image/png')
    blob = dataURItoBlob(imgURI)
    var w = window.open(URL.createObjectURL(blob), 'test.png');
});


function init() {
    max_x = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    max_y = basic_data.bounds[1][1] - basic_data.bounds[0][1];
    if (basic_data['type'] == "Mesh3") {
        max = 2 * Math.max(max_x, max_y);
    } else {
        max = Math.max(max_x, max_y);
    }
    // max = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    c = {
        x: (basic_data.bounds[1][0] + basic_data.bounds[0][0]) / 2,
        y: (basic_data.bounds[1][1] + basic_data.bounds[0][1]) / 2
    }
    const sc = 1000;
    frustumSize = sc + sc / 10;
    var aspect = 1;
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true, preserveDrawingBuffer: true });
    camera = new THREE.OrthographicCamera(frustumSize / -2 * max, frustumSize / 2 * max, frustumSize / 2 * max, frustumSize / -2 * max, 1, 8 * frustumSize * max);
    // camera = new THREE.PerspectiveCamera(frustumSize * max / 20, 1, 1, frustumSize * max);
    // camera = new THREE.PerspectiveCamera(40, window.innerWidth / window.innerHeight, frustumSize * max);
    if (basic_data['type'] == "Mesh3") {
        camera.position.set(frustumSize, frustumSize, frustumSize);
    } else {
        camera.position.set(0, 0, frustumSize);
    }
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    scene = new THREE.Scene();
    axes = new THREE.AxesHelper(frustumSize);
    axes.material.linewidth = 5;
    axes.position.set(-c.x * sc, -c.y * sc, 0); //move center

    var theatre = document.getElementById("new_plot")
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(sc, (max_y / max_x) * sc);
    renderer.setClearAlpha(0);
    renderer.domElement.id = 'canvas_3d';

    theatre.appendChild(renderer.domElement);
    scene.add(new THREE.AmbientLight(0x404040));
    scene.add(new THREE.AmbientLight(0x404040));
    scene.add(new THREE.AmbientLight(0x404040));
    scene.add(new THREE.HemisphereLight(0xffffff, 0x080820, 1));

    renderer.localClippingEnabled = true;


}

function mydraw3d() {
    scene.remove(mesh);
    scene.remove(mesh_border);
    scene.remove(axes);
    const sc = 1000;
    const xc = sc;
    const yc = sc / (max_y / max_x);
    var zc = sc / (3 * (minmax_data[1].u - minmax_data[0].u));
    if (!isFinite(zc)) {
        zc = sc;
    }

    var geometry = new THREE.BufferGeometry();
    var vertices = [];
    var normals = [];
    var material = new THREE.ShaderMaterial({
        uniforms: {
            color1: {
                value: new THREE.Color(cprofile[0])
            },
            color2: {
                value: new THREE.Color(cprofile[1])
            },
            color3: {
                value: new THREE.Color(cprofile[2])
            },
            color4: {
                value: new THREE.Color(cprofile[3])
            },
            color5: {
                value: new THREE.Color(cprofile[4])
            },
            bboxMin: {
                value: {
                    'x': vertex_data[minmax_data[0].id].x * xc,
                    'y': vertex_data[minmax_data[0].id].y * yc,
                    'z': minmax_data[0].u * zc
                }
            },
            bboxMax: {
                value: {
                    'x': vertex_data[minmax_data[1].id].x * xc,
                    'y': vertex_data[minmax_data[1].id].x * yc,
                    'z': minmax_data[1].u * zc
                }
            }
        },
        vertexShader: `
                            uniform vec3 bboxMin;
                            uniform vec3 bboxMax;
                        
                            varying vec2 vUv;

                            void main() {
                                if ((bboxMax.z - bboxMin.z) > 0.0){
                                    vUv.y = (position.z - bboxMin.z) / (bboxMax.z - bboxMin.z);
                                }else{
                                    vUv.y = 0.0;
                                }
                                gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0);
                            }
                        `,
        fragmentShader: `
                            uniform vec3 color1;
                            uniform vec3 color2;
                            uniform vec3 color3;
                            uniform vec3 color4;
                            uniform vec3 color5;

                        
                            varying vec2 vUv;
                            
                            void main() {
                                if (vUv.y >= 0.75){
                                    gl_FragColor = vec4(mix(color4, color5, 4.0 * vUv.y - 3.0), 1.0);
                                }else if (vUv.y >= 0.5 && vUv.y < 0.75){
                                    gl_FragColor = vec4(mix(color3, color4, 4.0 * vUv.y - 2.0), 1.0);
                                }else if (vUv.y >= 0.25 && vUv.y < 0.5){
                                    gl_FragColor = vec4(mix(color2, color3, 4.0 * vUv.y - 1.0), 1.0);
                                }else{
                                    gl_FragColor = vec4(mix(color1, color2, 4.0 * vUv.y), 1.0);
                                }
                            }
                        `,
        wireframe: $("#wireframe").is(':checked'),
        wireframeLinewidth: 1 / 500 * sc,
    });
    material.needsUpdate = true


    mesh_data.forEach((e) => {
        var v0 = new THREE.Vector3((e[0].x - c.x) * xc, (e[0].y - c.y) * yc, e[0].u * zc);
        var v1 = new THREE.Vector3((e[1].x - c.x) * xc, (e[1].y - c.y) * yc, e[1].u * zc);
        var v2 = new THREE.Vector3((e[2].x - c.x) * xc, (e[2].y - c.y) * yc, e[2].u * zc);
        var vg = new THREE.Vector3();
        vg.add(v0).add(v1).add(v2).multiplyScalar(1 / 3);

        var triangle = new THREE.Triangle(v0, v1, v2);
        var normal = triangle.getNormal(vg);

        // var material = new THREE.MeshLambertMaterial({wireframe: true, wireframeLinewidth: 1, side: THREE.DoubleSide });
        vertices.push(triangle.a.x, triangle.a.y, triangle.a.z);
        vertices.push(triangle.b.x, triangle.b.y, triangle.b.z);
        vertices.push(triangle.c.x, triangle.c.y, triangle.c.z);
        normals.push(normal.x, normal.y, normal.z);
        normals.push(normal.x, normal.y, normal.z);
        normals.push(normal.x, normal.y, normal.z);

    })

    geometry.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geometry.computeBoundingSphere();
    // var material = new THREE.MeshLambertMaterial({ wireframe: $("#wireframe").is(':checked'), wireframeLinewidth: 1 });
    mesh = new THREE.Mesh(geometry, material);
    scene.add(mesh);

    if (!$("#wireframe").is(':checked') && $("#mesh3d").is(':checked')) {
        var material = new THREE.LineBasicMaterial({
            color: 0x000000,
            linewidth: 1,
            linecap: 'round', //ignored by WebGLRenderer
            linejoin: 'round' //ignored by WebGLRenderer
        });
        var wireframe = new THREE.WireframeGeometry(geometry);
        mesh_border = new THREE.LineSegments(wireframe, material);
        mesh_border.material.depthTest = false;
        mesh_border.material.opacity = 0.25;
        mesh_border.material.transparent = true;
        scene.add(mesh_border);
    }

    if ($("#axes3d").is(':checked')) {
        scene.add(axes);
    }

}

function mydraw3dmesh() {
    alert("start")
    scene.remove(mesh);
    scene.remove(mesh_border);
    scene.remove(axes);
    scene.remove(helpers);
    max_z = basic_data.bounds[1][2] - basic_data.bounds[0][2];

    const sc = 1000;
    const xc = sc;
    const yc = sc / (max_y / max_x);
    const zc = sc / (max_z / max_x);
    renderer.localClippingEnabled = true;
    // const yc = sc / (max_y / max_x);
    // var zc = sc / (3 * (minmax_data[1].u - minmax_data[0].u));
    // if (!isFinite(zc)) {
    //     zc = sc;
    // }
    var intersec_x;
    var intersec_y;
    var intersec_z;
    if ($("#intersec_check").is(':checked')) {
        intersec_x = Number($("#intersection_x").val()) * sc;
        intersec_y = Number($("#intersection_y").val()) * sc;
        intersec_z = Number($("#intersection_z").val()) * sc;
    } else {
        intersec_x = 1 * sc;
        intersec_y = 1 * sc;
        intersec_z = 1 * sc;
    }
    clipPlanes = [
        new THREE.Plane(new THREE.Vector3(1, 0, 1).normalize(), intersec_x),
        new THREE.Plane(new THREE.Vector3(0, -1, 0).normalize(), intersec_y),
        new THREE.Plane(new THREE.Vector3(0, 0, -1).normalize(), intersec_z)
    ];

    c['z'] = (basic_data.bounds[1][2] + basic_data.bounds[0][2]) / 2
    axes.position.set(-c.x * sc, -c.y * sc, -c.z * sc); //move center
    var geometry = new THREE.BufferGeometry();
    var vertices = [];
    var normals = [];
    var color_template = [
        new SVG.Color(cprofile[0]).to(cprofile[1]),
        new SVG.Color(cprofile[1]).to(cprofile[2]),
        new SVG.Color(cprofile[2]).to(cprofile[3]),
        new SVG.Color(cprofile[3]).to(cprofile[4])
    ]
    // console.log(color_template)
    var color = new THREE.Color();
    var colors = [];
    var triangle = [];
    var vn = new THREE.Vector3();
    var vortex;


    mesh_data.forEach((e) => {

        var tempcolor = [];
        if ((minmax_data[1].u - minmax_data[0].u) > 0) {
            for (let index = 0; index < 4; index++) {
                var cr = 4 * (e[index].u - minmax_data[0].u) / (minmax_data[1].u - minmax_data[0].u);
                var ci = Math.floor(cr);
                if (ci == 4) {
                    ci = ci - 1;
                }
                tempcolor.push(color_template[ci].at(cr - ci).toHex());
                // color.setRGB(data[i][3] / max_kappa, 0, 0);
                // colors.push(color.r, color.g, color.b);
            }

        }

        triangle = [];
        vortex = [new THREE.Vector3((e[0].x - c.x) * xc, (e[0].y - c.y) * yc, (e[0].z - c.z) * zc),
        new THREE.Vector3((e[1].x - c.x) * xc, (e[1].y - c.y) * yc, (e[1].z - c.z) * zc),
        new THREE.Vector3((e[2].x - c.x) * xc, (e[2].y - c.y) * yc, (e[2].z - c.z) * zc),
        new THREE.Vector3((e[3].x - c.x) * xc, (e[3].y - c.y) * yc, (e[3].z - c.z) * zc)]

        triangle.push(new THREE.Triangle(vortex[2], vortex[1], vortex[0]));
        color.set(tempcolor[2]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[1]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[0]);
        colors.push(color.r, color.g, color.b);
        triangle.push(new THREE.Triangle(vortex[0], vortex[3], vortex[2]));
        color.set(tempcolor[0]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[3]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[2]);
        colors.push(color.r, color.g, color.b);
        triangle.push(new THREE.Triangle(vortex[1], vortex[3], vortex[0]));
        color.set(tempcolor[1]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[3]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[0]);
        colors.push(color.r, color.g, color.b);
        triangle.push(new THREE.Triangle(vortex[2], vortex[3], vortex[1]));
        color.set(tempcolor[2]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[3]);
        colors.push(color.r, color.g, color.b);
        color.set(tempcolor[1]);
        colors.push(color.r, color.g, color.b);

        // var material = new THREE.MeshLambertMaterial({wireframe: true, wireframeLinewidth: 1, side: THREE.DoubleSide });
        for (let index = 0; index < 4; index++) {
            triangle[index].getNormal(vn);
            vertices.push(triangle[index].a.x, triangle[index].a.y, triangle[index].a.z);
            vertices.push(triangle[index].b.x, triangle[index].b.y, triangle[index].b.z);
            vertices.push(triangle[index].c.x, triangle[index].c.y, triangle[index].c.z);
            normals.push(vn.x, vn.y, vn.z);
            normals.push(vn.x, vn.y, vn.z);
            normals.push(vn.x, vn.y, vn.z);
        }

    })

    // var material = new THREE.PointsMaterial({ size: 8, vertexColors: true });

    // material.needsUpdate = true
    // itemSize = 3 because there are 3 values (components) per vertex
    // geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
    // var material = new THREE.MeshBasicMaterial({ color: 0xff0000, side: THREE.DoubleSide});

    geometry.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));

    geometry.computeBoundingSphere();

    if ($("#intersec_check").is(':checked')) {
        var material = new THREE.MeshPhysicalMaterial({
            color: 0xf0f0f0,
            vertexColors: true, wireframe: $("#wireframe").is(':checked'),
            wireframeLinewidth: 1 / 500 * sc,
            clippingPlanes: clipPlanes,
            clipIntersection: $("#intersec_check").is(':checked')
        });
        helpers = new THREE.Group();
        helpers.add(new THREE.PlaneHelper(clipPlanes[0], sc, 0xff0000));
        helpers.add(new THREE.PlaneHelper(clipPlanes[1], sc, 0x00ff00));
        helpers.add(new THREE.PlaneHelper(clipPlanes[2], sc, 0x0000ff));
        helpers.visible = $("#intersec_check").is(':checked');
        scene.add(helpers);
    } else {
        var material = new THREE.MeshPhysicalMaterial({
            color: 0xf0f0f0,
            vertexColors: true, wireframe: $("#wireframe").is(':checked'),
            wireframeLinewidth: 1 / 500 * sc,
        });
    }

    mesh = new THREE.Mesh(geometry, material);

    scene.add(mesh);


    if (!$("#wireframe").is(':checked') && $("#mesh3d").is(':checked')) {
        var material = new THREE.LineBasicMaterial({
            color: 0x000000,
            linewidth: 1,
            linecap: 'round', //ignored by WebGLRenderer
            linejoin: 'round', //ignored by WebGLRenderer
            clippingPlanes: clipPlanes,
            clipIntersection: $("#intersec_check").is(':checked')
        });
        var wireframe = new THREE.WireframeGeometry(geometry);
        mesh_border = new THREE.LineSegments(wireframe, material);
        mesh_border.material.depthTest = false;
        mesh_border.material.opacity = 0.25;
        mesh_border.material.transparent = true;
        scene.add(mesh_border);
    }

    if ($("#axes3d").is(':checked')) {
        scene.add(axes);
    }

}

function animate() {
    if (!pause) {
        next();
        if (now_file >= total_file) {
            play();
        }
    }
    setTimeout(function () {
        requestAnimationFrame(animate);
    }, 50);

    renderer.render(scene, camera);
}


function view(x, y, z) {
    camera.position.set(x, y, z);
    controls.update();
}

