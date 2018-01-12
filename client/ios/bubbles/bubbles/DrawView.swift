//
//  DrawView.swift
//  bubbles
//
//  Created by Valentin Palade on 06/01/2018.
//  Copyright © 2018 Valentin Palade. All rights reserved.
//

import UIKit
import os.log

class DrawView: UIView {
    var position: CGPoint = CGPoint(x: 0, y: 0)
    let local_diameter: CGFloat = 60.0
    let remote_diameter: CGFloat = 30.0
    
    var has_touch: false
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder: NSCoder){
        super.init(coder: coder)
        position = super.center
    }
    
    public func onLoad() {
        position = super.center
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        has_touch = true
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        engine_move(position.x, position.y)
        self.setNeedsDisplay()
    }
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        has_touch = false
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        engine_move(position.x, position.y)
        self.setNeedsDisplay()
    }
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        position = touches.first!.location(in: self)
        os_log("DrawView: x = %@; y = %@", log: OSLog.default, type: .debug, position.x.description, position.y.description)
        engine_move(position.x, position.y)
        self.setNeedsDisplay()
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        //nothing here yet
    }
    
    func drawCircle(x: Int32, y: Int32, color: Int32, diameter: CGFloat){
        os_log("drawCircle: x = %@; y = %@; c = %@; d = %@", log: OSLog.default, type: .debug, x.description, y.description, c.description, diameter.description)
        //TODO:
    } 
    
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        engine_plot_start()
        
        let my_color: engine_plot_my_color()
        
        while engine_plot_end() != 0{
            let x: engine_plot_x()
            let y: engine_plot_y()
            let c: engine_plot_color()
            
            drawCircle(x, y, c, remote_diameter)
            engine_plot_next()
        }
        
        drawCircle(position.x, positio.y, my_color, local_diameter)
        engine_plot_done()
        // Drawing code
//         var path = UIBezierPath()
//         path = UIBezierPath(ovalIn: CGRect(x: position.x - diameter/2, y: position.y - diameter/2, width: diameter, height: diameter))
//         UIColor.black.setStroke()
//         UIColor.black.setFill()
//         path.lineWidth = 5
//         path.stroke()
//         path.fill()
        
    }
    func autoMove(x: Int32, y: Int32){
        if !has_touch{
            position.x = x
            position.y = y
            engine_move(x, y)
            self.setNeedsDisplay()
        }
    }
}
